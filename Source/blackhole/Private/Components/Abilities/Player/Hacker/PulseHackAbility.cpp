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

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	// Get all actors in pulse radius
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	FVector CharacterLocation = Character->GetActorLocation();

	GetWorld()->SweepMultiByChannel(
		HitResults,
		CharacterLocation,
		CharacterLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(HackRadius),
		QueryParams
	);

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
		
		if (UResourceManager* ResourceMgr = GetWorld()->GetGameInstance()->GetSubsystem<UResourceManager>())
		{
			// Reduce WP corruption (negative value to AddWillPower reduces WP)
			ResourceMgr->AddWillPower(-CleansingAmount);
			
			UE_LOG(LogTemp, Log, TEXT("Pulse Hack: Hit %d enemies, cleansed %.1f WP corruption"), EnemiesHit, CleansingAmount);
		}
	}

	// Spawn visual effect
	if (PulseEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			PulseEffect,
			CharacterLocation,
			FRotator::ZeroRotator,
			FVector(1.0f)
		);
	}

	// Play sound
	if (PulseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PulseSound, CharacterLocation);
	}

	// Debug visualization
#if WITH_EDITOR
	DrawDebugSphere(GetWorld(), CharacterLocation, HackRadius, 32, FColor::Cyan, false, 1.0f);
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

	GetWorld()->GetTimerManager().SetTimer(
		RestoreHandle,
		RestoreDelegate,
		SlowDuration,
		false
	);

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