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
	
	// Consume WP first
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		if (!ResMgr->ConsumeWillPower(Cost))
		{
			return; // Failed to consume WP
		}
	}
	
	// Call parent to handle cooldown and heat generation
	Super::Execute();
	
	// Apply the movement
	ApplyMovement(Character);
	
	// Set timer to stop movement after duration
	if (MovementDuration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			MovementTimerHandle,
			this,
			&UUtilityAbility::StopMovement,
			MovementDuration,
			false
		);
	}
}

bool UUtilityAbility::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	// Check resource manager for WP and overheat
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		return ResMgr->HasEnoughWillPower(Cost) && !ResMgr->IsOverheated();
	}
	
	return false;
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
	return Cast<ACharacter>(GetOwner());
}