#include "Components/Abilities/Player/Hacker/DataSpikeAbility.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Enemy/BaseEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/HitStopManager.h"
#include "Utils/ErrorHandling.h"

UDataSpikeAbility::UDataSpikeAbility()
{
	// Ability costs and cooldown (per GDD)
	WPCost = GameplayConfig::Abilities::DataSpike::WP_COST;
	// HeatCost removed - heat system no longer exists
	Cooldown = GameplayConfig::Abilities::DataSpike::COOLDOWN;
	// HeatGenerationMultiplier removed - heat system no longer exists
	
	// Ensure this is NOT a basic ability
	bIsBasicAbility = false;
}

bool UDataSpikeAbility::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	return true;
}

void UDataSpikeAbility::Execute()
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
	
	// Fire the data spike projectile
	FireProjectile(false);
}

void UDataSpikeAbility::ExecuteUltimate()
{
	// Ultimate Data Spike - "System Corruption"
	// Pierces all enemies and applies enhanced data corruption
	UE_LOG(LogTemp, Warning, TEXT("ULTIMATE DATA SPIKE: System Corruption!"));
	
	Super::ExecuteUltimate();
	
	// Fire enhanced projectile
	FireProjectile(true);
	
	// Apply WP cleanse
	if (UResourceManager* ResourceMgr = GetGameInstanceSubsystemSafe<UResourceManager>(this))
	{
		ResourceMgr->AddWillPower(-UltimateWPCleanse);
		UE_LOG(LogTemp, Warning, TEXT("Ultimate Data Spike: Cleansed %.0f WP!"), 
			UltimateWPCleanse);
	}
}

void UDataSpikeAbility::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up all active DOT timers
	if (UWorld* World = GetWorld())
	{
		if (World)
		{
			for (auto& DOTPair : ActiveDOTs)
			{
				if (DOTPair.Value.DOTTimerHandle.IsValid())
				{
					World->GetTimerManager().ClearTimer(DOTPair.Value.DOTTimerHandle);
				}
			}
		}
	}
	ActiveDOTs.Empty();
	
	Super::EndPlay(EndPlayReason);
}

void UDataSpikeAbility::FireProjectile(bool bIsUltimate)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("DataSpikeAbility: FireProjectile failed - no owner"));
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character || !GetWorld())
	{
		return;
	}

	FVector Start = GetProjectileStart();
	FVector Direction = GetProjectileDirection();
	FVector End = Start + (Direction * ProjectileRange);

	// Perform line trace to simulate projectile
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	QueryParams.bReturnPhysicalMaterial = false;

	// Use multi-trace to handle piercing
	bool bHit = false;
	if (UWorld* World = GetWorld())
	{
		bHit = World->LineTraceMultiByChannel(
			HitResults,
			Start,
			End,
			ECC_Pawn,
			QueryParams
		);
	}

	int32 RemainingPierces = bIsUltimate ? 
		GameplayConfig::Abilities::DataSpike::ULTIMATE_PIERCE_COUNT : 
		PierceCount;

	// Process all hits along the projectile path
	TArray<AActor*> HitActors; // Track hit actors to avoid duplicate hits
	for (const FHitResult& Hit : HitResults)
	{
		if (RemainingPierces <= 0)
		{
			break;
		}

		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor))
		{
			continue; // Skip if already hit this actor
		}

		// Only affect enemies
		if (HitActor->ActorHasTag("Enemy") || HitActor->IsA<ABaseEnemy>())
		{
			ProcessProjectileHit(Hit, RemainingPierces, bIsUltimate);
			HitActors.Add(HitActor);
		}
	}

	// Play fire sound
	if (FireSound)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(World, FireSound, Start);
		}
	}

	// Spawn projectile trail effect
	if (ProjectileEffect)
	{
		FVector EffectEnd = HitResults.Num() > 0 ? HitResults.Last().Location : End;
		
		// For now, we'll use a simple effect at the start point
		// TODO: Implement proper projectile trail between Start and EffectEnd
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				ProjectileEffect,
				Start,
				Direction.Rotation(),
				FVector(1.0f)
			);
		}
	}

	// Debug visualization
#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		FColor LineColor = bIsUltimate ? FColor::Purple : FColor::Green;
		DrawDebugLine(World, Start, End, LineColor, false, 2.0f, 0, 3.0f);
		
		// Draw hit points
		for (const FHitResult& Hit : HitResults)
		{
			if (Hit.GetActor() && (Hit.GetActor()->ActorHasTag("Enemy") || Hit.GetActor()->IsA<ABaseEnemy>()))
			{
				DrawDebugSphere(World, Hit.Location, 25.0f, 12, FColor::Red, false, 1.0f);
			}
		}
	}
#endif
}

void UDataSpikeAbility::ProcessProjectileHit(const FHitResult& HitResult, int32& RemainingPierces, bool bIsUltimate)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		return;
	}

	ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor);
	if (!Enemy)
	{
		return;
	}

	// Apply immediate damage
	UIntegrityComponent* TargetIntegrity = Enemy->FindComponentByClass<UIntegrityComponent>();
	if (TargetIntegrity)
	{
		float FinalDamage = ProjectileDamage * GetDamageMultiplier();
		if (bIsUltimate)
		{
			FinalDamage *= GameplayConfig::Abilities::DataSpike::ULTIMATE_DAMAGE_MULT;
		}
		
		TargetIntegrity->TakeDamage(FinalDamage);
		
		// Trigger hit stop
		if (UHitStopManager* HitStopMgr = GetWorldSubsystemSafe<UHitStopManager>(this))
		{
			HitStopMgr->RequestMediumHitStop();
		}
		
		UE_LOG(LogTemp, Log, TEXT("Data Spike hit %s for %.1f damage"), 
			*Enemy->GetName(), FinalDamage);
	}

	// Apply data corruption DOT
	ApplyDataCorruption(Enemy, bIsUltimate);

	// Spawn hit effect
	if (HitEffect)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				HitEffect,
				HitResult.Location,
				FRotator::ZeroRotator,
				FVector(1.0f)
			);
		}
	}

	// Play hit sound
	if (HitSound)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(World, HitSound, HitResult.Location);
		}
	}

	// Reduce remaining pierces
	RemainingPierces--;
}

void UDataSpikeAbility::ApplyDataCorruption(ABaseEnemy* Enemy, bool bIsUltimate)
{
	if (!Enemy || !IsValid(Enemy))
	{
		return;
	}

	TWeakObjectPtr<ABaseEnemy> WeakEnemy(Enemy);
	
	// Remove existing DOT if any
	RemoveDOTFromEnemy(Enemy);

	// Create new DOT effect
	float DOTDamageAmount = DOTDamage * GetDamageMultiplier();
	if (bIsUltimate)
	{
		DOTDamageAmount *= GameplayConfig::Abilities::DataSpike::ULTIMATE_DOT_MULT;
	}

	FDataCorruption Corruption(DOTDamageAmount, DOTDuration, DOTTickRate);
	ActiveDOTs.Add(WeakEnemy, Corruption);

	// Set up DOT timer with weak pointer capture
	FTimerDelegate DOTDelegate;
	DOTDelegate.BindLambda([this, WeakEnemy]()
	{
		if (WeakEnemy.IsValid())
		{
			OnDOTTick(WeakEnemy.Get());
		}
		else
		{
			// Enemy was destroyed, clean up timer
			if (ActiveDOTs.Contains(WeakEnemy))
			{
				if (UWorld* World = GetWorld())
				{
					World->GetTimerManager().ClearTimer(ActiveDOTs[WeakEnemy].DOTTimerHandle);
				}
				ActiveDOTs.Remove(WeakEnemy);
			}
		}
	});

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ActiveDOTs[WeakEnemy].DOTTimerHandle,
			DOTDelegate,
			DOTTickRate,
			true, // Loop
			DOTTickRate // First tick delay
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("DataSpikeAbility: Cannot set DOT timer - no valid world"));
	}

	// Spawn DOT visual effect on enemy
	if (DOTEffect)
	{
		// TODO: Attach effect to enemy for duration
		UGameplayStatics::SpawnEmitterAttached(
			DOTEffect,
			Enemy->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	UE_LOG(LogTemp, Log, TEXT("Applied data corruption to %s: %.1f damage every %.1fs for %.1fs"), 
		*Enemy->GetName(), DOTDamageAmount, DOTTickRate, DOTDuration);
}

void UDataSpikeAbility::OnDOTTick(ABaseEnemy* Enemy)
{
	if (!Enemy || !IsValid(Enemy))
	{
		RemoveDOTFromEnemy(Enemy);
		return;
	}
	
	TWeakObjectPtr<ABaseEnemy> WeakEnemy(Enemy);
	if (!ActiveDOTs.Contains(WeakEnemy))
	{
		return;
	}

	FDataCorruption& Corruption = ActiveDOTs[WeakEnemy];
	
	// Apply DOT damage
	UIntegrityComponent* TargetIntegrity = Enemy->FindComponentByClass<UIntegrityComponent>();
	if (TargetIntegrity)
	{
		TargetIntegrity->TakeDamage(Corruption.DamagePerTick);
		
		UE_LOG(LogTemp, Verbose, TEXT("Data corruption tick on %s: %.1f damage (%d ticks remaining)"), 
			*Enemy->GetName(), Corruption.DamagePerTick, Corruption.TicksRemaining - 1);
	}

	// Reduce remaining ticks
	Corruption.TicksRemaining--;
	
	// Remove DOT if finished
	if (Corruption.TicksRemaining <= 0)
	{
		RemoveDOTFromEnemy(Enemy);
	}
}

void UDataSpikeAbility::RemoveDOTFromEnemy(ABaseEnemy* Enemy)
{
	if (!Enemy)
	{
		return;
	}
	
	TWeakObjectPtr<ABaseEnemy> WeakEnemy(Enemy);
	if (!ActiveDOTs.Contains(WeakEnemy))
	{
		return;
	}

	// Clear timer
	if (UWorld* World = GetWorld())
	{
		if (World && ActiveDOTs[WeakEnemy].DOTTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(ActiveDOTs[WeakEnemy].DOTTimerHandle);
		}
	}

	// Remove from tracking
	ActiveDOTs.Remove(WeakEnemy);
}

FVector UDataSpikeAbility::GetProjectileDirection() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ForwardVector;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		return FVector::ForwardVector;
	}

	// Use camera direction for aiming if this is the player
	if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Character))
	{
		if (UCameraComponent* Camera = PlayerOwner->GetCameraComponent())
		{
			return Camera->GetForwardVector();
		}
	}

	// Fallback to actor forward vector
	return Character->GetActorForwardVector();
}

FVector UDataSpikeAbility::GetProjectileStart() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	// Use camera location for aiming if this is the player
	if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Character))
	{
		if (UCameraComponent* Camera = PlayerOwner->GetCameraComponent())
		{
			return Camera->GetComponentLocation();
		}
	}

	// Fallback to actor location with slight forward offset
	FVector ActorLocation = Character->GetActorLocation();
	FVector ForwardOffset = Character->GetActorForwardVector() * 100.0f; // 1 meter forward
	return ActorLocation + ForwardOffset;
}