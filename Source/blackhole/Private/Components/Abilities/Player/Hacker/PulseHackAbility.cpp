#include "Components/Abilities/Player/Hacker/PulseHackAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/BaseEnemy.h"
#include "Systems/ResourceManager.h"

UPulseHackAbility::UPulseHackAbility()
{
	// Ability costs and cooldown (per GDD)
	Cost = 10.0f; // Legacy field
	StaminaCost = 5.0f; // New dual resource system
	WPCost = 10.0f; // New dual resource system
	HeatCost = 0.0f; // Hacker abilities don't consume heat
	Cooldown = 8.0f;
	HeatGenerationMultiplier = 0.5f;
}

bool UPulseHackAbility::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	return true;
}

void UPulseHackAbility::Execute()
{
	if (!CanExecute())
	{
		return;
	}

	Super::Execute();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("PulseHackAbility: Execute failed - no owner"));
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		return;
	}

	// Get all actors in pulse radius
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	FVector CharacterLocation = Character->GetActorLocation();

	if (UWorld* World = GetWorld())
	{
		World->SweepMultiByChannel(
			HitResults,
			CharacterLocation,
			CharacterLocation,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(HackRadius),
			QueryParams
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PulseHackAbility: Cannot sweep - no valid world"));
		return;
	}

	// Track enemies hit for WP refund
	int32 EnemiesHit = 0;

	// Apply slow to all enemies in radius
	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || (!HitActor->ActorHasTag("Enemy") && !HitActor->IsA<ABaseEnemy>()))
		{
			continue;
		}

		if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor))
		{
			ApplySlowToEnemy(Enemy);
			EnemiesHit++;
		}
	}

	// Calculate and apply WP corruption cleansing
	if (EnemiesHit > 0)
	{
		float CleansingAmount = FMath::Min(EnemiesHit * WPRefundPerEnemy, MaxWPRefund);
		
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
				{
					// Reduce WP corruption (negative value to AddWillPower reduces WP)
					ResourceMgr->AddWillPower(-CleansingAmount);
					
					UE_LOG(LogTemp, Log, TEXT("Pulse Hack: Hit %d enemies, cleansed %.1f WP corruption"), EnemiesHit, CleansingAmount);
				}
			}
		}
	}

	// Spawn visual effect
	if (PulseEffect)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				PulseEffect,
				CharacterLocation,
				FRotator::ZeroRotator,
				FVector(1.0f)
			);
		}
	}

	// Play sound
	if (PulseSound)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(World, PulseSound, CharacterLocation);
		}
	}

	// Debug visualization
#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		DrawDebugSphere(World, CharacterLocation, HackRadius, 32, FColor::Cyan, false, 1.0f);
	}
#endif
}

void UPulseHackAbility::ApplySlowToEnemy(ABaseEnemy* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// Store original speed
	float OriginalSpeed = Movement->MaxWalkSpeed;

	// Apply slow
	Movement->MaxWalkSpeed = OriginalSpeed * SlowAmount;

	// Set timer to restore speed
	FTimerHandle RestoreHandle;
	FTimerDelegate RestoreDelegate;
	RestoreDelegate.BindUObject(this, &UPulseHackAbility::RestoreEnemySpeed, Enemy, OriginalSpeed);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RestoreHandle,
			RestoreDelegate,
			SlowDuration,
			false
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PulseHackAbility: Cannot set timer - no valid world"));
	}

	// Visual feedback on enemy
	// TODO: Add slow effect on enemy (particle/material change)
}

void UPulseHackAbility::RestoreEnemySpeed(ABaseEnemy* Enemy, float OriginalSpeed)
{
	if (!Enemy || !IsValid(Enemy))
	{
		return;
	}

	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = OriginalSpeed;
	}
}

void UPulseHackAbility::ExecuteUltimate()
{
	// Ultimate Pulse Hack - "System Overload"
	// Massive pulse that stuns all enemies on screen and cleanses 50 WP
	UE_LOG(LogTemp, Warning, TEXT("ULTIMATE PULSE HACK: System Overload!"));
	
	Super::ExecuteUltimate();
	
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("PulseHackAbility: ExecuteUltimate failed - no owner"));
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		return;
	}
	
	// Ultimate version: Massive radius and full stun
	float UltimateRadius = HackRadius * GameplayConfig::Abilities::PulseHack::ULTIMATE_RADIUS_MULT; // Multiplied radius
	float StunDuration = GameplayConfig::Abilities::PulseHack::ULTIMATE_STUN_DURATION; // Full stun instead of slow
	float UltimateWPCleanse = GameplayConfig::Abilities::PulseHack::ULTIMATE_WP_CLEANSE; // Massive WP cleanse
	
	// Get all actors in massive radius
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	
	FVector CharacterLocation = Character->GetActorLocation();
	
	if (UWorld* World = GetWorld())
	{
		World->SweepMultiByChannel(
			HitResults,
			CharacterLocation,
			CharacterLocation,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(UltimateRadius),
			QueryParams
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PulseHackAbility: Cannot sweep - no valid world"));
		return;
	}
	
	// Stun all enemies in radius
	int32 EnemiesAffected = 0;
	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || (!HitActor->ActorHasTag("Enemy") && !HitActor->IsA<ABaseEnemy>()))
		{
			continue;
		}
		
		if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor))
		{
			// Full stun - set movement speed to 0
			UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
			if (Movement)
			{
				float OriginalSpeed = Movement->MaxWalkSpeed;
				Movement->MaxWalkSpeed = 0.0f; // Complete stun
				
				// Set timer to restore movement
				FTimerHandle RestoreHandle;
				FTimerDelegate RestoreDelegate;
				RestoreDelegate.BindUObject(this, &UPulseHackAbility::RestoreEnemySpeed, Enemy, OriginalSpeed);
				
				if (UWorld* World = GetWorld())
				{
					World->GetTimerManager().SetTimer(
						RestoreHandle,
						RestoreDelegate,
						StunDuration,
						false
					);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("PulseHackAbility: Cannot set timer - no valid world"));
				}
				
				EnemiesAffected++;
				
				UE_LOG(LogTemp, Warning, TEXT("Ultimate Pulse stunned %s for %.1f seconds!"), 
					*Enemy->GetName(), StunDuration);
			}
		}
	}
	
	// Apply massive WP cleanse
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
			{
				ResourceMgr->AddWillPower(-UltimateWPCleanse);
				UE_LOG(LogTemp, Warning, TEXT("Ultimate Pulse Hack: Cleansed %.0f WP, stunned %d enemies!"), 
					UltimateWPCleanse, EnemiesAffected);
			}
		}
	}
	
	// Spawn massive visual effect
	if (PulseEffect)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				PulseEffect,
				CharacterLocation,
				FRotator::ZeroRotator,
				FVector(4.0f) // Larger effect
			);
		}
	}
	
	// Play enhanced sound
	if (PulseSound)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(World, PulseSound, CharacterLocation, 2.0f); // Louder
		}
	}
	
	// Debug visualization
#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		DrawDebugSphere(World, CharacterLocation, UltimateRadius, 64, FColor::Purple, false, 2.0f);
	}
#endif
}