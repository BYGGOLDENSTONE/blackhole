#include "Components/Abilities/Player/Hacker/SystemOverrideAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/BaseEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/HitStopManager.h"

USystemOverrideAbility::USystemOverrideAbility()
{
	// Ability costs and cooldown (per GDD)
	WPCost = GameplayConfig::Abilities::SystemOverride::WP_COST;
	// HeatCost removed - heat system no longer exists
	Cooldown = GameplayConfig::Abilities::SystemOverride::COOLDOWN;
	// HeatGenerationMultiplier removed - heat system no longer exists
	
	// This is a high-level ability, not basic
	bIsBasicAbility = false;
}

bool USystemOverrideAbility::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	return true;
}

void USystemOverrideAbility::Execute()
{
	if (!CanExecute())
	{
		return;
	}

	Super::Execute();
	
	// IMPORTANT: If we're in ultimate mode, the base class already called ExecuteUltimate()
	// We should not continue with normal execution
	if (IsInUltimateMode())
	{
		return;
	}
	
	// Perform normal system override
	PerformSystemOverride();
}

void USystemOverrideAbility::ExecuteUltimate()
{
	// System Override is itself an ultimate-level ability
	// When used in ultimate mode, it has enhanced effects
	UE_LOG(LogTemp, Warning, TEXT("ULTIMATE SYSTEM OVERRIDE: Total System Shutdown!"));
	
	Super::ExecuteUltimate();
	
	// Enhanced system override with longer duration and more damage
	PerformSystemOverride();
	
	// Additional WP cleanse for ultimate version
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
			{
				float UltimateWPCleanse = WPCleanse * UltimateWPCleanseMultiplier;
				ResourceMgr->AddWillPower(-UltimateWPCleanse);
				UE_LOG(LogTemp, Warning, TEXT("Ultimate System Override: Enhanced WP cleanse %.0f!"), 
					UltimateWPCleanse);
			}
		}
	}
}

void USystemOverrideAbility::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up all active disable timers
	if (UWorld* World = GetWorld())
	{
		if (World)
		{
			for (auto& DisablePair : DisabledEnemies)
			{
				if (DisablePair.Value.RestoreTimerHandle.IsValid())
				{
					World->GetTimerManager().ClearTimer(DisablePair.Value.RestoreTimerHandle);
				}
			}
		}
	}
	DisabledEnemies.Empty();
	
	Super::EndPlay(EndPlayReason);
}

void USystemOverrideAbility::PerformSystemOverride()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("SystemOverrideAbility: PerformSystemOverride failed - no owner"));
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character || !GetWorld())
	{
		return;
	}

	FVector CharacterLocation = Character->GetActorLocation();
	
	// Get all enemies in radius
	TArray<ABaseEnemy*> EnemiesInRange = GetEnemiesInRadius();
	
	if (EnemiesInRange.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("System Override: No enemies in range"));
		return;
	}

	// Apply effects to all enemies in range
	int32 EnemiesAffected = 0;
	for (ABaseEnemy* Enemy : EnemiesInRange)
	{
		if (IsValid(Enemy))
		{
			// Apply damage
			UIntegrityComponent* TargetIntegrity = Enemy->FindComponentByClass<UIntegrityComponent>();
			if (TargetIntegrity)
			{
				float FinalDamage = AreaDamage * GetDamageMultiplier();
				TargetIntegrity->TakeDamage(FinalDamage);
				
				UE_LOG(LogTemp, Log, TEXT("System Override damaged %s for %.1f"), 
					*Enemy->GetName(), FinalDamage);
			}

			// Disable enemy systems
			DisableEnemy(Enemy);
			EnemiesAffected++;
		}
	}

	// Apply WP cleanse based on enemies affected
	if (EnemiesAffected > 0)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
				{
					ResourceMgr->AddWillPower(-WPCleanse);
					UE_LOG(LogTemp, Warning, TEXT("System Override: Cleansed %.0f WP, affected %d enemies"), 
						WPCleanse, EnemiesAffected);
				}
			}
		}
	}

	// Trigger heavy hit stop for the massive effect
	if (UWorld* World = GetWorld())
	{
		if (UHitStopManager* HitStopMgr = World->GetSubsystem<UHitStopManager>())
		{
			HitStopMgr->RequestHeavyHitStop();
		}
	}

	// Play activation sound
	if (ActivationSound)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(World, ActivationSound, CharacterLocation);
		}
	}

	// Spawn main override effect
	if (OverrideEffect)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				OverrideEffect,
				CharacterLocation,
				FRotator::ZeroRotator,
				FVector(2.0f) // Larger effect
			);
		}
	}

	// Debug visualization
#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		DrawDebugSphere(World, CharacterLocation, EffectRadius, 32, FColor::Orange, false, 3.0f);
		
		// Draw lines to affected enemies
		for (ABaseEnemy* Enemy : EnemiesInRange)
		{
			if (IsValid(Enemy))
			{
				DrawDebugLine(World, CharacterLocation, Enemy->GetActorLocation(), 
					FColor::Red, false, 2.0f, 0, 3.0f);
			}
		}
	}
#endif
}

void USystemOverrideAbility::DisableEnemy(ABaseEnemy* Enemy)
{
	if (!Enemy || !IsValid(Enemy))
	{
		return;
	}

	TWeakObjectPtr<ABaseEnemy> WeakEnemy(Enemy);
	
	// Remove existing disable if any
	if (DisabledEnemies.Contains(WeakEnemy))
	{
		if (UWorld* World = GetWorld())
		{
			if (World && DisabledEnemies[WeakEnemy].RestoreTimerHandle.IsValid())
			{
				World->GetTimerManager().ClearTimer(DisabledEnemies[WeakEnemy].RestoreTimerHandle);
			}
		}
	}

	// Store original movement speed
	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	float OriginalSpeed = Movement->MaxWalkSpeed;
	
	// Disable movement (set to 0)
	Movement->MaxWalkSpeed = 0.0f;
	
	// Store disable information
	FSystemDisable DisableInfo(DisableDuration, OriginalSpeed);
	DisabledEnemies.Add(WeakEnemy, DisableInfo);

	// Set timer to restore enemy
	FTimerDelegate RestoreDelegate;
	RestoreDelegate.BindUObject(this, &USystemOverrideAbility::RestoreEnemy, Enemy);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DisabledEnemies[WeakEnemy].RestoreTimerHandle,
			RestoreDelegate,
			DisableDuration,
			false
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SystemOverrideAbility: Cannot set timer - no valid world"));
	}

	// Play disable sound
	if (DisableSound)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(World, DisableSound, Enemy->GetActorLocation());
		}
	}

	// Spawn disable effect on enemy
	if (DisableEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			DisableEffect,
			Enemy->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	UE_LOG(LogTemp, Log, TEXT("System Override disabled %s for %.1f seconds"), 
		*Enemy->GetName(), DisableDuration);
}

void USystemOverrideAbility::RestoreEnemy(ABaseEnemy* Enemy)
{
	if (!Enemy || !IsValid(Enemy))
	{
		// Clean up if enemy is invalid - find and remove by weak pointer
		for (auto It = DisabledEnemies.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid() || It.Key().Get() == Enemy)
			{
				It.RemoveCurrent();
			}
		}
		return;
	}
	
	TWeakObjectPtr<ABaseEnemy> WeakEnemy(Enemy);
	if (!DisabledEnemies.Contains(WeakEnemy))
	{
		return;
	}

	FSystemDisable& DisableInfo = DisabledEnemies[WeakEnemy];
	
	// Restore movement speed
	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = DisableInfo.OriginalMaxWalkSpeed;
		UE_LOG(LogTemp, Log, TEXT("System Override restored %s (speed: %.1f)"), 
			*Enemy->GetName(), DisableInfo.OriginalMaxWalkSpeed);
	}

	// Remove from tracking
	DisabledEnemies.Remove(WeakEnemy);
}

TArray<ABaseEnemy*> USystemOverrideAbility::GetEnemiesInRadius() const
{
	TArray<ABaseEnemy*> EnemiesInRange;
	
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return EnemiesInRange;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character || !GetWorld())
	{
		return EnemiesInRange;
	}

	FVector CharacterLocation = Character->GetActorLocation();

	// Get all actors in radius
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	bool bFoundOverlaps = false;
	if (UWorld* World = GetWorld())
	{
		bFoundOverlaps = World->OverlapMultiByChannel(
			OverlapResults,
			CharacterLocation,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(EffectRadius),
			QueryParams
		);
	}

	if (bFoundOverlaps)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* HitActor = Result.GetActor();
			if (HitActor && (HitActor->ActorHasTag("Enemy") || HitActor->IsA<ABaseEnemy>()))
			{
				if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor))
				{
					EnemiesInRange.Add(Enemy);
				}
			}
		}
	}

	return EnemiesInRange;
}