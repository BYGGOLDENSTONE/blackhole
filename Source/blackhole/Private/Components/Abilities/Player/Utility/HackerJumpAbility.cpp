#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UHackerJumpAbility::UHackerJumpAbility()
{
	// Set hacker path
	PathType = ECharacterPath::Hacker;
	
	// Jump costs stamina but no cooldown
	Cost = 10.0f; // Legacy field
	StaminaCost = 10.0f; // Per GDD: 10 stamina cost
	WPCost = 0.0f; // Utility abilities don't add WP corruption
	HeatCost = 0.0f; // Hacker abilities don't generate heat
	Cooldown = 0.0f; // No cooldown for jumps
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
	// Don't check additional resource requirements for jumps
	if (!GetCharacterOwner() || !CachedMovement)
	{
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: No owner or movement component"));
		return false;
	}
	
	// Check if on cooldown (shouldn't happen with 0 cooldown)
	if (IsOnCooldown())
	{
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: On cooldown"));
		return false;
	}
	
	// Check jump count
	bool bCanJump = CanJump();
	if (!bCanJump)
	{
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: CanJump returned false (CurrentJump: %d, Max: %d)"), CurrentJumpCount, MaxJumpCount);
	}
	return bCanJump;
}

void UHackerJumpAbility::ApplyMovement(ACharacter* Character)
{
	if (!Character || !CachedMovement || !CanJump())
	{
		return;
	}
	
	// For first jump, use the character's built-in jump
	if (CurrentJumpCount == 0 && CachedMovement->IsMovingOnGround())
	{
		Character->Jump();
		CurrentJumpCount = 1;
		
		// Boost air control for better maneuverability
		CachedMovement->AirControl = OriginalAirControl * AirControlBoost;
	}
	else if (CurrentJumpCount < MaxJumpCount)
	{
		// For double jump, manually apply velocity
		CurrentJumpCount++;
		
		// Reset Z velocity for clean double jump
		CachedMovement->Velocity.Z = 0.0f;
		
		// Apply jump velocity
		CachedMovement->Velocity.Z = JumpVelocity;
		
		// Notify the movement component
		CachedMovement->SetMovementMode(MOVE_Falling);
		
		// Set character jumping state
		Character->JumpCurrentCount = CurrentJumpCount;
		
		#if WITH_EDITOR
		// Debug visualization
		FVector CharLocation = Character->GetActorLocation();
		DrawDebugLine(GetWorld(), CharLocation, CharLocation + FVector(0, 0, JumpVelocity), FColor::Cyan, false, 1.0f, 0, 3.0f);
		
		// Show jump count
		FString JumpText = FString::Printf(TEXT("Jump %d/%d"), CurrentJumpCount, MaxJumpCount);
		DrawDebugString(GetWorld(), CharLocation + FVector(0, 0, 100), JumpText, nullptr, FColor::White, 1.0f);
		#endif
	}
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