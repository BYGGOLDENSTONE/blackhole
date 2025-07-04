#include "Components/Abilities/Player/Forge/HammerStrikeAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Enemy/BaseEnemy.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"

UHammerStrikeAbility::UHammerStrikeAbility()
{
	// Ability costs and cooldown (per GDD)
	Cost = 20.0f; // Legacy field
	StaminaCost = 15.0f; // New dual resource system
	WPCost = 0.0f; // Forge abilities don't consume WP
	HeatCost = 20.0f; // New dual resource system
	Cooldown = 6.0f;
	HeatGenerationMultiplier = 0.7f;
	
	bComboWindowActive = false;
	CurrentComboCount = 0;
}

bool UHammerStrikeAbility::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	return true;
}

void UHammerStrikeAbility::Execute()
{
	if (!CanExecute())
	{
		return;
	}

	// Check if we're in combo window
	if (bComboWindowActive && CurrentComboCount < MaxComboHits)
	{
		// This is a combo hit - don't consume WP or trigger cooldown
		CurrentComboCount++;
		UE_LOG(LogTemp, Log, TEXT("Hammer Strike Combo Hit #%d"), CurrentComboCount);
	}
	else
	{
		// This is the initial hit
		Super::Execute();
		CurrentComboCount = 1;
		bComboWindowActive = true;
		
		// Start combo window timer
		GetWorld()->GetTimerManager().SetTimer(
			ComboWindowTimer,
			this,
			&UHammerStrikeAbility::EndComboWindow,
			ComboWindowDuration,
			false
		);
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	FVector CharacterLocation = Character->GetActorLocation();
	FVector CharacterForward = Character->GetActorForwardVector();

	// Get all actors in strike range
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	// Perform a cone sweep in front of character
	FVector SweepStart = CharacterLocation;
	FVector SweepEnd = CharacterLocation + CharacterForward * StrikeRange;

	GetWorld()->SweepMultiByChannel(
		HitResults,
		SweepStart,
		SweepEnd,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(100.0f),
		QueryParams
	);

	bool bHitEnemy = false;

	// Apply effects to all hit enemies
	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || (!HitActor->ActorHasTag("Enemy") && !HitActor->IsA<ABaseEnemy>()))
		{
			continue;
		}

		if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor))
		{
			// Calculate damage with combo multiplier
			float FinalDamage = GetComboDamage();
			
			// Apply damage
			if (UIntegrityComponent* TargetIntegrity = Enemy->FindComponentByClass<UIntegrityComponent>())
			{
				TargetIntegrity->TakeDamage(FinalDamage);
			}

			// Apply stun on first hit or if not already stunned
			if (!StunnedEnemies.Contains(Enemy))
			{
				StunEnemy(Enemy);
			}

			bHitEnemy = true;
		}
	}

	// Visual effects
	if (StrikeEffect)
	{
		FVector EffectLocation = CharacterLocation + CharacterForward * 150.0f;
		FRotator EffectRotation = Character->GetActorRotation();
		
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			StrikeEffect,
			EffectLocation,
			EffectRotation,
			FVector(1.0f + CurrentComboCount * 0.2f) // Scale up with combo
		);
	}

	// Play sound
	if (StrikeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), StrikeSound, CharacterLocation);
	}

	// Camera shake
	if (StrikeCameraShake && bHitEnemy)
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->ClientStartCameraShake(StrikeCameraShake, CurrentComboCount * 0.5f);
		}
	}

	// Debug visualization
#if WITH_EDITOR
	DrawDebugCone(
		GetWorld(),
		CharacterLocation,
		CharacterForward,
		StrikeRange,
		FMath::DegreesToRadians(45.0f),
		FMath::DegreesToRadians(45.0f),
		8,
		FColor::Red,
		false,
		1.0f
	);
#endif

	UE_LOG(LogTemp, Log, TEXT("Hammer Strike executed - Combo #%d, Damage: %f"), CurrentComboCount, GetComboDamage());
}

void UHammerStrikeAbility::StunEnemy(ABaseEnemy* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	// Add to stunned list
	StunnedEnemies.Add(Enemy);

	// Disable enemy movement
	if (UCharacterMovementComponent* EnemyMovement = Enemy->GetCharacterMovement())
	{
		EnemyMovement->DisableMovement();
	}

	// Spawn stun effect
	if (StunEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			StunEffect,
			Enemy->GetRootComponent(),
			NAME_None,
			FVector(0, 0, Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	// Play stun sound
	if (StunSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), StunSound, Enemy->GetActorLocation());
	}

	// Set timer to remove stun
	FTimerHandle StunTimer;
	FTimerDelegate StunDelegate;
	StunDelegate.BindUObject(this, &UHammerStrikeAbility::RemoveStun, Enemy);
	
	GetWorld()->GetTimerManager().SetTimer(
		StunTimer,
		StunDelegate,
		StunDuration,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("Enemy stunned for %f seconds"), StunDuration);
}

void UHammerStrikeAbility::RemoveStun(ABaseEnemy* Enemy)
{
	if (!Enemy || !IsValid(Enemy))
	{
		return;
	}

	// Remove from stunned list
	StunnedEnemies.Remove(Enemy);

	// Re-enable movement
	if (UCharacterMovementComponent* EnemyMovement = Enemy->GetCharacterMovement())
	{
		EnemyMovement->SetMovementMode(MOVE_Walking);
	}

	UE_LOG(LogTemp, Log, TEXT("Enemy stun removed"));
}

void UHammerStrikeAbility::EndComboWindow()
{
	bComboWindowActive = false;
	CurrentComboCount = 0;
	
	// Clear stunned enemies list
	StunnedEnemies.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("Hammer Strike combo window ended"));
}

float UHammerStrikeAbility::GetComboDamage() const
{
	// Each combo hit increases damage
	float ComboMultiplier = 1.0f + ((CurrentComboCount - 1) * (ComboDamageMultiplier - 1.0f));
	return StrikeDamage * ComboMultiplier;
}