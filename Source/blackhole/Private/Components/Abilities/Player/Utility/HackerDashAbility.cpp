#include "Components/Abilities/Player/Utility/HackerDashAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Systems/ComboSystem.h"
#include "Camera/CameraComponent.h"

UHackerDashAbility::UHackerDashAbility()
{
	// Set hacker path
	PathType = ECharacterPath::Hacker;
	
	// Mark as basic ability - not affected by ultimate system
	bIsBasicAbility = true;
	
	// Override base parameters
	WPCost = 0.0f; // Utility abilities don't add WP corruption
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
	// Dash behavior: Uses movement input direction
	// - W + dash: Forward dash follows camera angle (can dash up/down in air)
	// - S/A/D + dash: Always horizontal regardless of camera angle
	// - No input: Dash forward relative to character facing
	// This provides intuitive dodging with special aerial forward dashing
	
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
	
	// Use movement input direction instead of camera direction
	FVector DashDirection;
	
	// Cast to player character once for reuse
	ABlackholePlayerCharacter* PlayerChar = Cast<ABlackholePlayerCharacter>(Character);
	
	// Get movement input for player
	if (PlayerChar)
	{
		// Get the last movement input vector
		// This returns the movement input already transformed to world space
		FVector InputVector = Character->GetLastMovementInputVector();
		
		// If there's movement input, use it directly
		if (InputVector.SizeSquared() > 0.01f)
		{
			// The input vector is already in world space
			DashDirection = InputVector;
			DashDirection.Normalize();
			
			// Special case: For forward dash (W key), allow vertical movement based on camera
			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				// Get camera forward direction (with pitch)
				FVector CameraForward = PC->GetControlRotation().Vector();
				
				// Get camera forward without pitch for comparison
				FRotator FlatRotation = PC->GetControlRotation();
				FlatRotation.Pitch = 0;
				FVector FlatCameraForward = FlatRotation.Vector();
				FlatCameraForward.Normalize();
				
				// Check if dash direction is primarily forward (aligned with camera forward)
				float ForwardDot = FVector::DotProduct(DashDirection, FlatCameraForward);
				
				// If dashing mostly forward (W key) and in air, use camera pitch
				if (ForwardDot > 0.7f && !CachedMovement->IsMovingOnGround())
				{
					// Use full camera direction including pitch for vertical dashing
					DashDirection = CameraForward;
					DashDirection.Normalize();
				}
			}
		}
		else
		{
			// No movement input - dash forward relative to character facing
			DashDirection = Character->GetActorForwardVector();
		}
	}
	else
	{
		// Non-player characters use their forward direction
		DashDirection = Character->GetActorForwardVector();
	}
	
	// Ground dash constraints
	if (CachedMovement->IsMovingOnGround())
	{
		// Check if this is a forward dash (W key) by comparing with camera forward
		bool bIsForwardDash = false;
		if (APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController()))
		{
			FRotator FlatRotation = PC->GetControlRotation();
			FlatRotation.Pitch = 0;
			FVector FlatCameraForward = FlatRotation.Vector();
			FlatCameraForward.Normalize();
			
			float ForwardDot = FVector::DotProduct(DashDirection, FlatCameraForward);
			bIsForwardDash = ForwardDot > 0.7f;
		}
		
		// Only constrain to horizontal if NOT a forward dash
		if (!bIsForwardDash)
		{
			DashDirection.Z = 0.0f;
			DashDirection.Normalize();
		}
		
		// Add slight upward velocity to clear small obstacles
		CachedMovement->Velocity.Z = 200.0f;
	}
	
	// Store original friction
	OriginalFriction = CachedMovement->BrakingFrictionFactor;
	
	// Remove friction during dash for smooth movement
	CachedMovement->BrakingFrictionFactor = 0.0f;
	
	// Set velocity directly for instant dash
	CachedMovement->Velocity = DashDirection * DashSpeed;
	
	#if WITH_EDITOR
	// Debug visualization
	FVector Start = Character->GetActorLocation();
	FVector End = Start + (CachedMovement->Velocity * DashDuration);
	
	// Different colors based on dash type
	FColor DashColor;
	bool bHasInput = Character->GetLastMovementInputVector().SizeSquared() > 0.01f;
	bool bIsVerticalDash = false;
	
	// Check if this is a vertical forward dash
	if (bHasInput)
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			FRotator FlatRotation = PC->GetControlRotation();
			FlatRotation.Pitch = 0;
			FVector FlatCameraForward = FlatRotation.Vector();
			FlatCameraForward.Normalize();
			
			float ForwardDot = FVector::DotProduct(DashDirection.GetSafeNormal(), FlatCameraForward);
			bIsVerticalDash = ForwardDot > 0.7f && FMath::Abs(DashDirection.Z) > 0.1f;
		}
	}
	
	if (!bHasInput)
	{
		// No input - fallback dash (orange)
		DashColor = FColor::Orange;
	}
	else if (bIsVerticalDash)
	{
		// Forward dash with vertical component (purple)
		DashColor = FColor::Purple;
	}
	else if (CachedMovement->IsMovingOnGround())
	{
		// Ground dash with input (green)
		DashColor = FColor::Green;
	}
	else
	{
		// Air dash with input (cyan)
		DashColor = FColor::Cyan;
	}
	
	DrawDebugLine(GetWorld(), Start, End, DashColor, false, 2.0f, 0, 5.0f);
	
	// Show dash direction arrow
	DrawDebugDirectionalArrow(GetWorld(), Start, End, 120.0f, DashColor, false, 2.0f, 0, 3.0f);
	
	// Show input vector for debugging
	if (bHasInput)
	{
		FVector InputVec = Character->GetLastMovementInputVector() * 200.0f;
		DrawDebugLine(GetWorld(), Start, Start + InputVec, FColor::Yellow, false, 2.0f, 0, 2.0f);
	}
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