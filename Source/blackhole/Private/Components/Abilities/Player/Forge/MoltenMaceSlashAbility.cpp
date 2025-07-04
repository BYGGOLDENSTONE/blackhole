#include "Components/Abilities/Player/Forge/MoltenMaceSlashAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Components/CapsuleComponent.h"

UMoltenMaceSlashAbility::UMoltenMaceSlashAbility()
{
	// Ability costs and cooldown (per GDD)
	Cost = 30.0f; // Legacy field
	StaminaCost = 20.0f; // New dual resource system
	WPCost = 0.0f; // Forge abilities don't consume WP
	HeatCost = 30.0f; // New dual resource system
	Cooldown = 5.0f;
	HeatGenerationMultiplier = 0.8f;
}

bool UMoltenMaceSlashAbility::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	return true;
}

void UMoltenMaceSlashAbility::Execute()
{
	if (!CanExecute())
	{
		return;
	}

	Super::Execute();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	FVector CharacterLocation = Character->GetActorLocation();
	FVector CharacterForward = Character->GetActorForwardVector();

	// Get all actors in slash range
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	GetWorld()->SweepMultiByChannel(
		HitResults,
		CharacterLocation,
		CharacterLocation + CharacterForward * SlashRange,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(SlashRange),
		QueryParams
	);

	// Track if we hit anything
	bool bHitEnemy = false;

	// Apply damage to all enemies in cone
	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || (!HitActor->ActorHasTag("Enemy") && !HitActor->IsA<ABaseEnemy>()))
		{
			continue;
		}

		// Check if enemy is within the slash cone
		if (!IsInSlashCone(HitActor->GetActorLocation(), CharacterLocation, CharacterForward))
		{
			continue;
		}

		if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor))
		{
			// Apply initial damage
			if (UIntegrityComponent* TargetIntegrity = Enemy->FindComponentByClass<UIntegrityComponent>())
			{
				TargetIntegrity->TakeDamage(SlashDamage);
			}

			// Apply stagger
			if (UCharacterMovementComponent* EnemyMovement = Enemy->GetCharacterMovement())
			{
				// Disable movement for stagger duration
				EnemyMovement->DisableMovement();
				
				// Re-enable after stagger
				FTimerHandle StaggerHandle;
				FTimerDelegate StaggerDelegate;
				StaggerDelegate.BindLambda([EnemyMovement]()
				{
					if (IsValid(EnemyMovement))
					{
						EnemyMovement->SetMovementMode(MOVE_Walking);
					}
				});
				
				GetWorld()->GetTimerManager().SetTimer(
					StaggerHandle,
					StaggerDelegate,
					StaggerDuration,
					false
				);
			}

			// Apply burn effect
			ApplyBurnDamage(Enemy);

			// Play impact effect on enemy
			if (ImpactSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, Enemy->GetActorLocation());
			}

			bHitEnemy = true;
		}
	}

	// Spawn slash visual effect
	if (SlashEffect)
	{
		FRotator EffectRotation = Character->GetActorRotation();
		FVector EffectLocation = CharacterLocation + CharacterForward * 100.0f;
		
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			SlashEffect,
			EffectLocation,
			EffectRotation,
			FVector(1.0f)
		);
	}

	// Play slash sound
	if (SlashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SlashSound, CharacterLocation);
	}

	// Debug visualization
#if WITH_EDITOR
	// Draw cone
	float HalfAngleRad = FMath::DegreesToRadians(SlashAngle * 0.5f);
	
	// Draw cone edges
	FVector LeftEdge = CharacterForward.RotateAngleAxis(-SlashAngle * 0.5f, FVector::UpVector) * SlashRange;
	FVector RightEdge = CharacterForward.RotateAngleAxis(SlashAngle * 0.5f, FVector::UpVector) * SlashRange;
	
	DrawDebugLine(GetWorld(), CharacterLocation, CharacterLocation + LeftEdge, FColor::Red, false, 1.0f, 0, 3.0f);
	DrawDebugLine(GetWorld(), CharacterLocation, CharacterLocation + RightEdge, FColor::Red, false, 1.0f, 0, 3.0f);
	DrawDebugLine(GetWorld(), CharacterLocation + LeftEdge, CharacterLocation + RightEdge, FColor::Red, false, 1.0f, 0, 3.0f);
#endif
}

bool UMoltenMaceSlashAbility::IsInSlashCone(const FVector& TargetLocation, const FVector& CharacterLocation, const FVector& CharacterForward) const
{
	FVector ToTarget = (TargetLocation - CharacterLocation).GetSafeNormal();
	float DotProduct = FVector::DotProduct(CharacterForward, ToTarget);
	float Angle = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
	
	return Angle <= (SlashAngle * 0.5f);
}

void UMoltenMaceSlashAbility::ApplyBurnDamage(ABaseEnemy* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	// Attach burn effect to enemy
	if (BurnEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			BurnEffect,
			Enemy->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}

	// Calculate number of burn ticks
	int32 TotalTicks = FMath::CeilToInt(BurnDuration);
	
	// Start burn damage
	DealBurnTick(Enemy, TotalTicks);
}

void UMoltenMaceSlashAbility::DealBurnTick(ABaseEnemy* Enemy, int32 TicksRemaining)
{
	if (!Enemy || !IsValid(Enemy) || TicksRemaining <= 0)
	{
		return;
	}

	// Apply burn damage
	if (UIntegrityComponent* TargetIntegrity = Enemy->FindComponentByClass<UIntegrityComponent>())
	{
		TargetIntegrity->TakeDamage(BurnDamagePerSecond);
	}

	// Schedule next tick
	FTimerHandle BurnHandle;
	FTimerDelegate BurnDelegate;
	BurnDelegate.BindUObject(this, &UMoltenMaceSlashAbility::DealBurnTick, Enemy, TicksRemaining - 1);

	GetWorld()->GetTimerManager().SetTimer(
		BurnHandle,
		BurnDelegate,
		1.0f,
		false
	);
}