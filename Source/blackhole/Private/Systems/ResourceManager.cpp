#include "Systems/ResourceManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Config/GameplayConfig.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Kismet/GameplayStatics.h"

void UResourceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Initialize default values from config
	MaxWP = GameplayConfig::Resources::MAX_WILLPOWER;
	CurrentWP = 100.0f; // WP starts at full (100 = full energy, 0 = death)
	PreviousWPThreshold = EResourceThreshold::Normal; // Starting at 100% WP = Normal state
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
	OnWPDepleted.Clear();
	
	Super::Deinitialize();
}

bool UResourceManager::ConsumeWillPower(float Amount)
{
	if (Amount < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ConsumeWillPower called with negative amount: %f"), Amount);
		return false;
	}
	
	// Allow consumption even if insufficient WP (to trigger critical state)
	float OldWP = CurrentWP;
	
	// Consume the WP, clamping at 0 (never go negative)
	CurrentWP = FMath::Max(0.0f, CurrentWP - Amount);
	
	UE_LOG(LogTemp, Warning, TEXT("ResourceManager::ConsumeWillPower: WP changed from %.1f to %.1f (consumed %.1f)"), 
		OldWP, CurrentWP, Amount);
	
	// Broadcast the change
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	SyncWillPowerComponent();
	
	// Check for threshold changes
	CheckWPThreshold();
	
	// Check for ultimate activation (WP reached 0)
	if (CurrentWP <= 0.0f && OldWP > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceManager: WP reached 0 - ACTIVATING ULTIMATE MODE"));
		OnWPDepleted.Broadcast(); // This now triggers ultimate mode, not death
	}
	
	return true;
}

void UResourceManager::AddWillPower(float Amount)
{
	// Don't allow WP restoration during critical state (WP at 0)
	// Player must use ultimate to exit critical state
	if (CurrentWP <= 0.0f && !bWPResetAuthorized)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceManager: Cannot add WP during critical state (WP at 0)"));
		return;
	}
	
	// Restore WP (from kills, pickups, etc)
	float OldWP = CurrentWP;
	CurrentWP = FMath::Clamp(CurrentWP + Amount, 0.0f, MaxWP);
	
	// Log significant WP changes
	if (FMath::Abs(CurrentWP - OldWP) > 0.1f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceManager: WP restored from %.1f to %.1f (delta: +%.1f)"), 
			OldWP, CurrentWP, CurrentWP - OldWP);
	}
	
	if (CurrentWP != OldWP)
	{
		OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
		SyncWillPowerComponent(); // Keep player's WillPowerComponent in sync
		CheckWPThreshold();
		
		// No special behavior at 100% - it's just full energy
		if (CurrentWP >= MaxWP && OldWP < MaxWP)
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceManager: WP fully restored to 100%%"));
		}
	}
}

EResourceThreshold UResourceManager::GetCurrentWPThreshold() const
{
	float Percent = GetWillPowerPercent();
	
	// New energy system: Low WP = danger, High WP = good
	if (Percent <= GameplayConfig::Resources::WP_LOW_THRESHOLD) // 20%
	{
		return EResourceThreshold::Critical; // 0-20% WP = Critical low energy
	}
	else if (Percent <= GameplayConfig::Resources::WP_BUFFED_THRESHOLD) // 50%
	{
		return EResourceThreshold::Warning; // 20-50% WP = Warning
	}
	else
	{
		return EResourceThreshold::Normal; // 50-100% WP = Normal/healthy state
	}
}

void UResourceManager::ResetResources()
{
	UE_LOG(LogTemp, Error, TEXT("ResourceManager::ResetResources called! WP was %.1f, resetting to 100"), CurrentWP);
	
	// CRITICAL: Block resets during critical timer state
	if (bInCriticalState)
	{
		UE_LOG(LogTemp, Error, TEXT("ResourceManager::ResetResources BLOCKED - Player is in critical timer state!"));
		const FString CallStack = FFrame::GetScriptCallstack();
		UE_LOG(LogTemp, Error, TEXT("Blocked reset call stack:\n%s"), *CallStack);
		return;
	}
	
	// Print call stack for debugging
	const FString CallStack = FFrame::GetScriptCallstack();
	UE_LOG(LogTemp, Error, TEXT("ResetResources call stack:\n%s"), *CallStack);
	
	CurrentWP = 100.0f; // Reset to full energy
	WPMaxReachedCount = 0; // Reset the counter
	
	// Clear overheat timer if active
	
	// Sync player's WillPowerComponent
	SyncWillPowerComponent();
	
	// Broadcast updates
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	
	// Reset threshold
	PreviousWPThreshold = EResourceThreshold::Normal;
}

void UResourceManager::ResetWPAfterMax()
{
	UE_LOG(LogTemp, Error, TEXT("!!! ResourceManager::ResetWPAfterMax CALLED !!!"));
	
	// Authorization check - prevent unwanted resets
	if (!bWPResetAuthorized)
	{
		UE_LOG(LogTemp, Error, TEXT("ResourceManager::ResetWPAfterMax called WITHOUT AUTHORIZATION - BLOCKING RESET!"));
		const FString CallStack = FFrame::GetScriptCallstack();
		UE_LOG(LogTemp, Error, TEXT("Unauthorized reset call stack:\n%s"), *CallStack);
		return;
	}
	
	// In energy system, we reset WP to 100 after using ultimate at 0% WP
	// No need to check if we're at max - we should be near 0 when ultimate is used
	
	UE_LOG(LogTemp, Error, TEXT("ResourceManager::ResetWPAfterMax - WP was %.1f, resetting to 100 (full energy)"), CurrentWP);
	
	// Print call stack for debugging
	const FString CallStack = FFrame::GetScriptCallstack();
	UE_LOG(LogTemp, Error, TEXT("Call stack:\n%s"), *CallStack);
	
	CurrentWP = 100.0f; // Reset to full energy
	SyncWillPowerComponent(); // Keep player's WillPowerComponent in sync
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	
	// Reset threshold to normal (since we're at 100%)
	PreviousWPThreshold = EResourceThreshold::Normal;
	CheckWPThreshold();
	
	// Clear authorization flag
	bWPResetAuthorized = false;
	
	UE_LOG(LogTemp, Error, TEXT("!!! ResourceManager: WP RESET TO 100 - This should only happen after respawn !!!"));
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

void UResourceManager::SyncWillPowerComponent()
{
	// Keep player's WillPowerComponent synchronized with ResourceManager's CurrentWP
	if (UWorld* World = GetWorld())
	{
		if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)))
		{
			if (UWillPowerComponent* WPComponent = Player->FindComponentByClass<UWillPowerComponent>())
			{
				// Update the component's value to match ResourceManager
				WPComponent->SetCurrentValue(CurrentWP);
				
				UE_LOG(LogTemp, VeryVerbose, TEXT("ResourceManager: Synced WillPowerComponent to %.1f"), CurrentWP);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ResourceManager: Player has no WillPowerComponent to sync!"));
			}
		}
	}
}

void UResourceManager::TakeDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("TakeDamage called with non-positive amount: %f"), DamageAmount);
		return;
	}
	
	// If already at 0 WP (critical state), no damage can be taken
	if (CurrentWP <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceManager::TakeDamage: WP already at 0, damage blocked"));
		return;
	}
	
	float OldWP = CurrentWP;
	
	// Damage reduces WP, clamping at 0 (never negative)
	CurrentWP = FMath::Max(0.0f, CurrentWP - DamageAmount);
	
	UE_LOG(LogTemp, Warning, TEXT("ResourceManager::TakeDamage: WP changed from %.1f to %.1f (damage: %.1f)"), 
		OldWP, CurrentWP, DamageAmount);
	
	// Broadcast the change
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	SyncWillPowerComponent();
	
	// Check for threshold changes
	CheckWPThreshold();
	
	// Check for ultimate activation (WP reached 0)
	if (CurrentWP <= 0.0f && OldWP > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceManager: WP reached 0 - ACTIVATING ULTIMATE MODE"));
		OnWPDepleted.Broadcast(); // This now triggers ultimate mode, not death
	}
}
