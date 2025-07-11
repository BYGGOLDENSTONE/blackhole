#include "Systems/ResourceManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Config/GameplayConfig.h"

void UResourceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Initialize default values from config
	MaxWP = GameplayConfig::Resources::MAX_WILLPOWER;
	CurrentWP = 0.0f; // WP starts at 0, gained from ability usage
	PreviousWPThreshold = EResourceThreshold::Normal; // Starting at 0% WP = Normal state
	CurrentPath = ECharacterPath::Hacker; // Default to Hacker
	WPMaxReachedCount = 0; // Track how many times WP reached 100%
}

void UResourceManager::Deinitialize()
{
	// Clear any active timers
	
	// Clear all delegate bindings to prevent crashes
	OnWillPowerChanged.Clear();
	OnWillPowerThresholdReached.Clear();
	OnWPMaxReached.Clear();
	
	Super::Deinitialize();
}

bool UResourceManager::ConsumeWillPower(float Amount)
{
	if (Amount < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ConsumeWillPower called with negative amount: %f"), Amount);
		return false;
	}
	
	// Check if we have enough WP
	if (CurrentWP < Amount)
	{
		UE_LOG(LogTemp, Verbose, TEXT("ConsumeWillPower: Insufficient WP (%.1f/%.1f required)"), CurrentWP, Amount);
		return false;
	}
	
	float OldWP = CurrentWP;
	
	// Consume the WP
	CurrentWP = FMath::Clamp(CurrentWP - Amount, 0.0f, MaxWP);
	
	UE_LOG(LogTemp, Warning, TEXT("ResourceManager::ConsumeWillPower: WP changed from %.1f to %.1f (consumed %.1f)"), 
		OldWP, CurrentWP, Amount);
	
	// Broadcast the change
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	
	// Check for threshold changes
	CheckWPThreshold();
	
	return true;
}

void UResourceManager::AddWillPower(float Amount)
{
	// Allow negative amounts for WP reduction (killing hacker enemies)
	float OldWP = CurrentWP;
	CurrentWP = FMath::Clamp(CurrentWP + Amount, 0.0f, MaxWP);
	
	// Log significant WP changes
	if (FMath::Abs(CurrentWP - OldWP) > 0.1f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceManager: WP changed from %.1f to %.1f (delta: %.1f)"), 
			OldWP, CurrentWP, CurrentWP - OldWP);
	}
	
	if (CurrentWP != OldWP)
	{
		OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
		CheckWPThreshold();
		
		// Check if WP reached 100%
		if (CurrentWP >= MaxWP && OldWP < MaxWP)
		{
			WPMaxReachedCount++;
			OnWPMaxReached.Broadcast(WPMaxReachedCount);
			
			UE_LOG(LogTemp, Error, TEXT("ResourceManager: WP reached 100%% (Count: %d) - NO AUTO RESET, waiting for ultimate use"), WPMaxReachedCount);
			
			// DO NOT automatically reset WP to 0 after reaching 100%
			// WP should only be reset when an ultimate ability is used via ThresholdManager
			
			// Ensure WP stays at max
			CurrentWP = MaxWP;
		}
	}
}

EResourceThreshold UResourceManager::GetCurrentWPThreshold() const
{
	float Percent = GetWillPowerPercent();
	
	// New system: 0-50% Normal, 50-100% Buffed, 100% Critical
	if (Percent >= 1.0f)
	{
		return EResourceThreshold::Critical; // 100% WP = Lose ability and reset
	}
	else if (Percent >= GameplayConfig::Resources::WP_BUFFED_THRESHOLD)
	{
		return EResourceThreshold::Buffed; // 50-100% WP = Abilities buffed
	}
	else
	{
		return EResourceThreshold::Normal; // 0-50% WP = Normal state
	}
}

void UResourceManager::ResetResources()
{
	UE_LOG(LogTemp, Error, TEXT("ResourceManager::ResetResources called! WP was %.1f, resetting to 0"), CurrentWP);
	
	CurrentWP = 0.0f; // Reset to 0 (good state)
	WPMaxReachedCount = 0; // Reset the counter
	
	// Clear overheat timer if active
	
	// Broadcast updates
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	
	// Reset threshold
	PreviousWPThreshold = EResourceThreshold::Normal;
}

void UResourceManager::ResetWPAfterMax()
{
	UE_LOG(LogTemp, Error, TEXT("!!! ResourceManager::ResetWPAfterMax CALLED !!!"));
	
	// Safety check - only reset if we're actually at max
	if (CurrentWP < MaxWP * 0.95f)
	{
		UE_LOG(LogTemp, Error, TEXT("ResourceManager::ResetWPAfterMax called but WP is only %.1f/%.1f - IGNORING!"), CurrentWP, MaxWP);
		return;
	}
	
	UE_LOG(LogTemp, Error, TEXT("ResourceManager::ResetWPAfterMax - WP was %.1f, resetting to 0"), CurrentWP);
	
	// Print call stack for debugging
	const FString CallStack = FFrame::GetScriptCallstack();
	UE_LOG(LogTemp, Error, TEXT("Call stack:\n%s"), *CallStack);
	
	CurrentWP = 0.0f;
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	
	// Reset threshold to normal
	PreviousWPThreshold = EResourceThreshold::Normal;
	CheckWPThreshold();
	
	UE_LOG(LogTemp, Error, TEXT("!!! ResourceManager: WP RESET TO 0 - This should only happen after ultimate ability use !!!"));
}

void UResourceManager::CheckWPThreshold()
{
	EResourceThreshold CurrentThreshold = GetCurrentWPThreshold();
	
	if (CurrentThreshold != PreviousWPThreshold)
	{
		OnWillPowerThresholdReached.Broadcast(CurrentThreshold);
		PreviousWPThreshold = CurrentThreshold;
	}
}

bool UResourceManager::WouldAddingWPCauseOverflow(float Amount) const
{
	// For Hacker path, we want to allow reaching 100% WP to trigger ultimate mode
	// Only block if it would go OVER 100%
	float ResultingWP = CurrentWP + Amount;
	
	// Allow abilities that would bring us exactly to 100% or less
	return ResultingWP > MaxWP;
}
