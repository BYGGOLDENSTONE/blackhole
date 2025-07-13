#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Systems/ComboSystem.h"

UHackerJumpAbility::UHackerJumpAbility()
{
	// Enable ticking for jump cooldown
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true; // IMPORTANT: Override parent's disabled tick
	
	// Set hacker path
	PathType = ECharacterPath::Hacker;
	
	// Mark as basic ability - not affected by ultimate system
	bIsBasicAbility = true;
	
	// Disable tick optimization since we need constant ticking for jump timer
	bTickOnlyWhenActive = false;
	
	// Jump costs stamina but no cooldown for the ability itself
	WPCost = 0.0f; // Utility abilities don't add WP corruption
	Cooldown = 0.0f; // No cooldown for jumps
	
	// Jump specifics
	JumpVelocity = 1200.0f;
	AirControlBoost = 2.0f;
	MaxJumpCount = 2;
	JumpCooldown = 0.1f; // Much shorter cooldown for responsive double jump
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
	
	// Skip parent's cooldown check since we manage our own jump cooldown
	// The parent's IsOnCooldown() was preventing double jumps
	
	// Check jump count and internal jump cooldown
	bool bCanJump = CanJump();
	if (!bCanJump)
	{
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: CanJump returned false (CurrentJump: %d, Max: %d, TimeSinceLastJump: %.2f)"), CurrentJumpCount, MaxJumpCount, TimeSinceLastJump);
	}
	return bCanJump;
}

void UHackerJumpAbility::Execute()
{
	// Skip parent's Execute to avoid cooldown application
	// We manage our own jump cooldown internally
	
	// Consume resources manually
	ConsumeAbilityResources();
	
	// Directly apply the movement without parent's resource/cooldown checks
	if (ACharacter* Character = GetCharacterOwner())
	{
		ApplyMovement(Character);
	}
	
	// Combo registration removed - now handled by individual combo components
}

void UHackerJumpAbility::ApplyMovement(ACharacter* Character)
{
	if (!Character || !CachedMovement || !CanJump())
	{
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: ApplyMovement failed - Character=%s, Movement=%s, CanJump=%s"),
			Character ? TEXT("Valid") : TEXT("NULL"),
			CachedMovement ? TEXT("Valid") : TEXT("NULL"),
			CanJump() ? TEXT("TRUE") : TEXT("FALSE"));
		return;
	}
	
	// For first jump, use the character's built-in jump
	if (CurrentJumpCount == 0 && CachedMovement->IsMovingOnGround())
	{
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: First jump - CurrentJumpCount=%d"), CurrentJumpCount);
		Character->Jump();
		CurrentJumpCount = 1;
		TimeSinceLastJump = 0.0f; // Reset jump timer
		
		// Boost air control for better maneuverability
		CachedMovement->AirControl = OriginalAirControl * AirControlBoost;
	}
	else if (CurrentJumpCount < MaxJumpCount && TimeSinceLastJump >= JumpCooldown)
	{
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: Double jump - CurrentJumpCount=%d, TimeSince=%.2f"), CurrentJumpCount, TimeSinceLastJump);
		// For double jump, manually apply velocity
		CurrentJumpCount++;
		TimeSinceLastJump = 0.0f; // Reset jump timer
		
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
	else
	{
		// Log why we can't jump
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: Cannot jump - CurrentJump=%d/%d, OnGround=%s, TimeSince=%.2f, Cooldown=%.2f"), 
			CurrentJumpCount, MaxJumpCount, 
			CachedMovement->IsMovingOnGround() ? TEXT("YES") : TEXT("NO"),
			TimeSinceLastJump, JumpCooldown);
	}
}

void UHackerJumpAbility::OnLanded(const FHitResult& Hit)
{
	// Reset jump count and timer when landing
	CurrentJumpCount = 0;
	TimeSinceLastJump = 0.0f;
	
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
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: CanJump - No CachedMovement"));
		return false;
	}
	
	// Can jump if on ground (first jump)
	if (CachedMovement->IsMovingOnGround())
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("HackerJump: CanJump - On ground, can jump"));
		return true;
	}
	
	// For double jump, check both jump count and cooldown
	if (CurrentJumpCount < MaxJumpCount)
	{
		// Must wait for cooldown between jumps
		bool bCanDoubleJump = TimeSinceLastJump >= JumpCooldown;
		UE_LOG(LogTemp, Warning, TEXT("HackerJump: CanJump - In air, CurrentJump=%d/%d, TimeSince=%.3f, Cooldown=%.3f, CanDoubleJump=%s"), 
			CurrentJumpCount, MaxJumpCount, TimeSinceLastJump, JumpCooldown, bCanDoubleJump ? TEXT("YES") : TEXT("NO"));
		return bCanDoubleJump;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("HackerJump: CanJump - Jump count exhausted (%d/%d)"), CurrentJumpCount, MaxJumpCount);
	return false;
}

void UHackerJumpAbility::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Update time since last jump
	if (CurrentJumpCount > 0)
	{
		TimeSinceLastJump += DeltaTime;
		
		// Debug tick logging removed - too verbose
	}
}