#include "Components/Abilities/Player/Forge/BlastChargeAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Enemy/BaseEnemy.h"
// #include "Engine/DestructibleActor.h" // Deprecated in UE5
#include "GameFramework/PlayerController.h"

UBlastChargeAbility::UBlastChargeAbility()
{
	// Ability costs and cooldown (per GDD)
	Cost = 25.0f; // Legacy field
	StaminaCost = 20.0f; // New dual resource system
	WPCost = 0.0f; // Forge abilities don't consume WP
	HeatCost = 25.0f; // New dual resource system
	Cooldown = 10.0f;
	HeatGenerationMultiplier = 0.7f;
	
	bIsCharging = false;
}

void UBlastChargeAbility::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up charge timer
	if (UWorld* World = GetWorld())
	{
		if (World && ChargeTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(ChargeTimerHandle);
		}
	}
	
	// Clean up active charge effect
	if (ActiveChargeEffect)
	{
		ActiveChargeEffect->DestroyComponent();
		ActiveChargeEffect = nullptr;
	}
	
	// Reset movement speed if still charging
	if (bIsCharging)
	{
		if (AActor* Owner = GetOwner())
		{
			if (ACharacter* Character = Cast<ACharacter>(Owner))
			{
				if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
				{
					Movement->MaxWalkSpeed *= 2.0f; // Restore speed (inverse of 0.5f)
				}
			}
		}
		bIsCharging = false;
	}
	
	Super::EndPlay(EndPlayReason);
}

bool UBlastChargeAbility::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	// Don't allow if already charging
	if (bIsCharging)
	{
		return false;
	}
	
	return true;
}

void UBlastChargeAbility::Execute()
{
	if (!CanExecute())
	{
		return;
	}

	Super::Execute();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("BlastChargeAbility: Execute failed - no owner"));
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		return;
	}

	// Start charging
	bIsCharging = true;

	// Slow character during charge
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed *= 0.5f; // 50% movement speed while charging
	}

	// Spawn charge effect
	if (ChargeEffect)
	{
		ActiveChargeEffect = UGameplayStatics::SpawnEmitterAttached(
			ChargeEffect,
			Character->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}

	// Play charge sound
	if (ChargeSound)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(World, ChargeSound, Character->GetActorLocation());
		}
	}

	// Set timer to execute blast
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ChargeTimerHandle,
			this,
			&UBlastChargeAbility::ExecuteBlast,
			ChargeTime,
			false
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BlastChargeAbility: Cannot set timer - no valid world"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Blast Charge charging for %f seconds"), ChargeTime);
}

void UBlastChargeAbility::ExecuteBlast()
{
	bIsCharging = false;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("BlastChargeAbility: ExecuteBlast failed - no owner"));
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		return;
	}

	FVector BlastLocation = Character->GetActorLocation();

	// Restore movement speed
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed *= 2.0f; // Restore from 50% slow
	}

	// Stop charge effect
	if (ActiveChargeEffect)
	{
		ActiveChargeEffect->DeactivateSystem();
		ActiveChargeEffect->DestroyComponent();
		ActiveChargeEffect = nullptr;
	}

	// Get all actors in blast radius
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	if (UWorld* World = GetWorld())
	{
		World->SweepMultiByChannel(
			HitResults,
			BlastLocation,
			BlastLocation,
			FQuat::Identity,
			ECC_WorldDynamic,
			FCollisionShape::MakeSphere(BlastRadius),
			QueryParams
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BlastChargeAbility: Cannot sweep - no valid world"));
		return;
	}

	// Apply effects to all hit actors
	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor)
		{
			continue;
		}

		// Damage enemies
		if (HitActor->ActorHasTag("Enemy") || HitActor->IsA<ABaseEnemy>())
		{
			if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
			{
				// Calculate falloff damage
				float Distance = FVector::Dist(HitActor->GetActorLocation(), BlastLocation);
				float DamageFalloff = 1.0f - (Distance / BlastRadius);
				float FinalDamage = BlastDamage * DamageFalloff;
				
				TargetIntegrity->TakeDamage(FinalDamage);
			}

			// Apply knockback
			ApplyKnockback(HitActor, BlastLocation);
		}
		// Damage destructible environment
		else if (bCanDestroyEnvironment)
		{
			// Destructible actors deprecated in UE5 - use generic damage or Chaos destruction
			// Check for actors tagged as destructible
			if (HitActor->ActorHasTag("Destructible") || HitActor->ActorHasTag("Breakable"))
			{
				// Apply damage to destructible environment
				// For now, just destroy the actor. In production, you would trigger
				// proper destruction effects through Chaos physics or custom logic
				HitActor->Destroy();
				
				// TODO: Implement proper Chaos destruction or custom break logic
				UE_LOG(LogTemp, Log, TEXT("Destroyed breakable actor: %s"), *HitActor->GetName());
			}
		}
	}

	// Spawn blast visual effect
	if (BlastEffect)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BlastEffect,
				BlastLocation,
				FRotator::ZeroRotator,
				FVector(1.0f)
			);
		}
	}

	// Play blast sound
	if (BlastSound)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(World, BlastSound, BlastLocation);
		}
	}

	// Camera shake
	if (BlastCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->ClientStartCameraShake(BlastCameraShake);
		}
	}

	// Debug visualization
#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		DrawDebugSphere(World, BlastLocation, BlastRadius, 32, FColor::Orange, false, 2.0f);
	}
#endif

	UE_LOG(LogTemp, Log, TEXT("Blast Charge executed at %s with radius %f"), *BlastLocation.ToString(), BlastRadius);
}

void UBlastChargeAbility::ApplyKnockback(AActor* Target, const FVector& BlastOrigin)
{
	if (!Target)
	{
		return;
	}

	FVector KnockbackDirection = (Target->GetActorLocation() - BlastOrigin).GetSafeNormal();
	KnockbackDirection.Z = 0.5f; // Add upward component
	KnockbackDirection.Normalize();

	// Calculate distance-based knockback
	float Distance = FVector::Dist(Target->GetActorLocation(), BlastOrigin);
	float KnockbackFalloff = 1.0f - (Distance / BlastRadius);
	float FinalKnockback = KnockbackForce * KnockbackFalloff;

	if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		TargetCharacter->LaunchCharacter(KnockbackDirection * FinalKnockback, true, true);
	}
	else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
	{
		if (PrimComp->IsSimulatingPhysics())
		{
			PrimComp->AddImpulseAtLocation(KnockbackDirection * FinalKnockback * 100.0f, Target->GetActorLocation());
		}
	}
}

void UBlastChargeAbility::DamageEnvironment(const FVector& BlastOrigin)
{
	// Additional environment damage logic can be added here
	// This could include:
	// - Breaking specific environmental pieces
	// - Triggering environmental hazards
	// - Creating blast marks/decals
}