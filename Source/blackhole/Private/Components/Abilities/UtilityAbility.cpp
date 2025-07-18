#include "Components/Abilities/UtilityAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Systems/ResourceManager.h"

UUtilityAbility::UUtilityAbility()
{
	// Utility abilities have lower costs and shorter cooldowns
	Cooldown = 1.0f;
}

void UUtilityAbility::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache movement component
	if (ACharacter* Character = GetCharacterOwner())
	{
		CachedMovement = Character->GetCharacterMovement();
	}
}

void UUtilityAbility::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	ACharacter* Character = GetCharacterOwner();
	if (!Character)
	{
		return;
	}
	
	// Utility abilities don't consume WP
	// The parent class will handle resource consumption
	
	// Call parent to handle resource consumption and cooldown
	Super::Execute();
	
	// Apply the movement
	ApplyMovement(Character);
	
	// Set timer to stop movement after duration
	if (MovementDuration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				MovementTimerHandle,
				this,
				&UUtilityAbility::StopMovement,
				MovementDuration,
				false
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UtilityAbility: Cannot set timer - no valid world"));
		}
	}
}

bool UUtilityAbility::CanExecute() const
{
	// Let the parent class handle resource checks
	// Utility abilities don't have additional WP/Heat requirements
	return Super::CanExecute();
}

void UUtilityAbility::ApplyMovement(ACharacter* Character)
{
	// Base implementation does nothing - override in derived classes
}

void UUtilityAbility::StopMovement()
{
	// Base implementation does nothing - override in derived classes if needed
}

ACharacter* UUtilityAbility::GetCharacterOwner() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	return Cast<ACharacter>(Owner);
}