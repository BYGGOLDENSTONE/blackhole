#include "Components/Abilities/Player/Utility/HackerDashAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Systems/ComboSystem.h"

UHackerDashAbility::UHackerDashAbility()
{
	// Set hacker path
	PathType = ECharacterPath::Hacker;
	
	// Mark as basic ability - not affected by ultimate system
	bIsBasicAbility = true;
	
	// Override base parameters
	Cost = 5.0f; // Legacy field
	StaminaCost = 5.0f; // Per GDD: 5 stamina cost
	WPCost = 0.0f; // Utility abilities don't add WP corruption
	HeatCost = 0.0f; // Hacker abilities don't generate heat
	Cooldown = 1.0f; // Per GDD: 1s cooldown
	MovementDuration = DashDuration;
	
	// Dash specifics
	DashSpeed = 3000.0f;
	DashDuration = 0.15f;
}

void UHackerDashAbility::Execute()
{
	// Call parent implementation
	Super::Execute();
	
	// Combo input registration is now handled by the player character
	// in UseDash() to avoid timing issues and double registration
}

void UHackerDashAbility::ApplyMovement(ACharacter* Character)
{
	if (!Character || !CachedMovement)
	{
		return;
	}
	
	// Safety check - ensure character is valid and not dead
	if (!IsValid(Character))
	{
		UE_LOG(LogTemp, Error, TEXT("HackerDashAbility: Character is invalid during ApplyMovement"));
		return;
	}
	
	// Get dash direction (prefer last movement input, fallback to camera forward)
	FVector DashDirection = Character->GetLastMovementInputVector();
	if (DashDirection.IsNearlyZero())
	{
		// Use camera forward direction
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			DashDirection = PC->GetControlRotation().Vector();
		}
		else
		{
			DashDirection = Character->GetActorForwardVector();
		}
	}
	
	// Keep dash horizontal
	DashDirection.Z = 0.0f;
	DashDirection.Normalize();
	
	// Store original friction
	OriginalFriction = CachedMovement->BrakingFrictionFactor;
	
	// Remove friction during dash for smooth movement
	CachedMovement->BrakingFrictionFactor = 0.0f;
	
	// Set velocity directly for instant dash
	CachedMovement->Velocity = DashDirection * DashSpeed;
	
	// Optional: Add slight upward velocity to clear small obstacles
	if (CachedMovement->IsMovingOnGround())
	{
		CachedMovement->Velocity.Z = 200.0f;
	}
	
	#if WITH_EDITOR
	// Debug visualization
	FVector Start = Character->GetActorLocation();
	FVector End = Start + (DashDirection * DashSpeed * DashDuration);
	DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 2.0f, 0, 5.0f);
	#endif
}

void UHackerDashAbility::StopMovement()
{
	if (IsValid(CachedMovement))
	{
		// Restore original friction
		CachedMovement->BrakingFrictionFactor = OriginalFriction;
		
		// Apply some braking to stop the dash smoothly
		CachedMovement->Velocity *= 0.5f;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HackerDashAbility: CachedMovement is invalid during StopMovement"));
	}
}