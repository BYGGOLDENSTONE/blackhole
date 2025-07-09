#include "Components/Abilities/UtilityAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Systems/ResourceManager.h"

UUtilityAbility::UUtilityAbility()
{
	// Utility abilities have lower costs and shorter cooldowns
	Cost = 5.0f;
	Cooldown = 1.0f;
	HeatGenerationMultiplier = 0.3f; // Generate less heat than combat abilities
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
	
	// Utility abilities only consume stamina, not WP or Heat
	// The parent class will handle stamina consumption
	
	// Call parent to handle stamina consumption and cooldown
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
	// Let the parent class handle resource checks (stamina)
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