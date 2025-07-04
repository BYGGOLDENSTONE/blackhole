#include "Components/Abilities/Player/Utility/HackerDashAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UHackerDashAbility::UHackerDashAbility()
{
	// Set hacker path
	PathType = ECharacterPath::Hacker;
	
	// Override base parameters
	Cost = 0.0f; // Legacy field - Hacker dash is free
	StaminaCost = 0.0f; // New dual resource system - free ability
	WPCost = 0.0f; // New dual resource system - free ability
	HeatCost = 0.0f; // New dual resource system - free ability
	Cooldown = 1.0f; // Per GDD: 1s cooldown
	MovementDuration = DashDuration;
	
	// Dash specifics
	DashSpeed = 3000.0f;
	DashDuration = 0.15f;
}

void UHackerDashAbility::ApplyMovement(ACharacter* Character)
{
	if (!Character || !CachedMovement)
	{
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
	if (CachedMovement)
	{
		// Restore original friction
		CachedMovement->BrakingFrictionFactor = OriginalFriction;
		
		// Apply some braking to stop the dash smoothly
		CachedMovement->Velocity *= 0.5f;
	}
}