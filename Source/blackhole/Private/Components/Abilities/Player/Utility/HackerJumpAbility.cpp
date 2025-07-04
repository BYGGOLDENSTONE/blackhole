#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UHackerJumpAbility::UHackerJumpAbility()
{
	// Set hacker path
	PathType = ECharacterPath::Hacker;
	
	// Jump has no cost or cooldown
	Cost = 0.0f; // Legacy field - free ability
	StaminaCost = 0.0f; // New dual resource system - free ability
	WPCost = 0.0f; // New dual resource system - free ability
	HeatCost = 0.0f; // New dual resource system - free ability
	Cooldown = 0.0f;
	HeatGenerationMultiplier = 0.0f; // No heat generation
	
	// Jump specifics
	JumpVelocity = 1200.0f;
	AirControlBoost = 2.0f;
	MaxJumpCount = 2;
}

void UHackerJumpAbility::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind to landing event
	if (ACharacter* Character = GetCharacterOwner())
	{
		Character->LandedDelegate.AddDynamic(this, &UHackerJumpAbility::OnLanded);
		
		if (CachedMovement)
		{
			OriginalAirControl = CachedMovement->AirControl;
		}
	}
}

bool UHackerJumpAbility::CanExecute() const
{
	// Don't check resource requirements for jumps
	if (!GetCharacterOwner() || !CachedMovement)
	{
		return false;
	}
	
	// Check if on cooldown (shouldn't happen with 0 cooldown, but just in case)
	if (IsOnCooldown())
	{
		return false;
	}
	
	// Check jump count
	return CanJump();
}

void UHackerJumpAbility::ApplyMovement(ACharacter* Character)
{
	if (!Character || !CachedMovement || !CanJump())
	{
		return;
	}
	
	// Increment jump count
	CurrentJumpCount++;
	
	// Apply jump velocity
	FVector JumpVector = FVector(0.0f, 0.0f, JumpVelocity);
	
	// For double jump, reset Z velocity first
	if (CurrentJumpCount > 1)
	{
		CachedMovement->Velocity.Z = 0.0f;
	}
	
	CachedMovement->Velocity += JumpVector;
	
	// Boost air control for better maneuverability
	CachedMovement->AirControl = OriginalAirControl * AirControlBoost;
	
	// Set character jumping state
	Character->bPressedJump = true;
	Character->JumpCurrentCount = CurrentJumpCount;
	
	#if WITH_EDITOR
	// Debug visualization
	FVector CharLocation = Character->GetActorLocation();
	DrawDebugLine(GetWorld(), CharLocation, CharLocation + JumpVector, FColor::Cyan, false, 1.0f, 0, 3.0f);
	
	// Show jump count
	FString JumpText = FString::Printf(TEXT("Jump %d/%d"), CurrentJumpCount, MaxJumpCount);
	DrawDebugString(GetWorld(), CharLocation + FVector(0, 0, 100), JumpText, nullptr, FColor::White, 1.0f);
	#endif
}

void UHackerJumpAbility::OnLanded(const FHitResult& Hit)
{
	// Reset jump count when landing
	CurrentJumpCount = 0;
	
	// Restore normal air control
	if (CachedMovement)
	{
		CachedMovement->AirControl = OriginalAirControl;
	}
	
	#if WITH_EDITOR
	UE_LOG(LogTemp, Log, TEXT("HackerJump: Landed, jump count reset"));
	#endif
}

bool UHackerJumpAbility::CanJump() const
{
	if (!CachedMovement)
	{
		return false;
	}
	
	// Can jump if on ground (first jump)
	if (CachedMovement->IsMovingOnGround())
	{
		return true;
	}
	
	// Can jump if in air and haven't exceeded max jumps
	return CurrentJumpCount < MaxJumpCount;
}