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
	MaxHeat = GameplayConfig::Resources::MAX_HEAT;
	CurrentHeat = 0.0f;
	HeatDissipationRate = GameplayConfig::Resources::HEAT_DISSIPATION_RATE;
	bIsOverheated = false;
	PreviousWPThreshold = EResourceThreshold::Normal; // Starting at 0% WP = Normal state
	CurrentPath = ECharacterPath::Hacker; // Default to Hacker
	WPMaxReachedCount = 0; // Track how many times WP reached 100%
}

void UResourceManager::Deinitialize()
{
	// Clear any active timers
	if (GetGameInstance() && GetGameInstance()->GetWorld())
	{
		GetGameInstance()->GetWorld()->GetTimerManager().ClearTimer(OverheatCooldownTimer);
	}
	
	// Clear all delegate bindings to prevent crashes
	OnWillPowerChanged.Clear();
	OnHeatChanged.Clear();
	OnWillPowerThresholdReached.Clear();
	OnWPMaxReached.Clear();
	OnOverheatShutdown.Clear();
	
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
	
	// Check if we're overheated (abilities disabled) - only in Forge path
	if (IsHeatSystemActive() && bIsOverheated)
	{
		UE_LOG(LogTemp, Verbose, TEXT("ConsumeWillPower: Cannot consume while overheated"));
		return false;
	}
	
	// Consume the WP
	CurrentWP = FMath::Clamp(CurrentWP - Amount, 0.0f, MaxWP);
	
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
	
	// No need to log WP changes - visible in UI
	
	if (CurrentWP != OldWP)
	{
		OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
		CheckWPThreshold();
		
		// Check if WP reached 100%
		if (CurrentWP >= MaxWP && OldWP < MaxWP)
		{
			WPMaxReachedCount++;
			OnWPMaxReached.Broadcast(WPMaxReachedCount);
			
			UE_LOG(LogTemp, Warning, TEXT("WP reached 100%% (Count: %d)"), WPMaxReachedCount);
			
			// Automatically reset WP to 0 after reaching 100%
			// This will be handled by ThresholdManager after ability loss
		}
	}
}

void UResourceManager::AddHeat(float Amount)
{
	// Heat system only active in Forge path
	if (!IsHeatSystemActive())
	{
		return;
	}
	
	if (Amount < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddHeat called with negative amount: %f"), Amount);
		return;
	}
	
	float OldHeat = CurrentHeat;
	CurrentHeat = FMath::Clamp(CurrentHeat + Amount, 0.0f, MaxHeat);
	
	if (CurrentHeat != OldHeat)
	{
		OnHeatChanged.Broadcast(CurrentHeat, MaxHeat);
		
		// Check for overheat
		if (!bIsOverheated && GetHeatPercent() >= GameplayConfig::Resources::HEAT_WARNING_PERCENT)
		{
			bIsOverheated = true;
			OnOverheatShutdown.Broadcast();
			
			// Start cooldown timer
			if (GetGameInstance() && GetGameInstance()->GetWorld())
			{
				GetGameInstance()->GetWorld()->GetTimerManager().SetTimer(
					OverheatCooldownTimer,
					this,
					&UResourceManager::EndOverheatShutdown,
					GameplayConfig::Resources::OVERHEAT_DURATION,
					false
				);
			}
		}
	}
}

bool UResourceManager::ConsumeHeat(float Amount)
{
	if (Amount < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ConsumeHeat called with negative amount: %f"), Amount);
		return false;
	}
	
	// Heat system only active in Forge path
	if (!IsHeatSystemActive())
	{
		return false;
	}
	
	// Check if we have enough heat
	if (CurrentHeat < Amount)
	{
		return false;
	}
	
	// Consume the heat
	float OldHeat = CurrentHeat;
	CurrentHeat = FMath::Clamp(CurrentHeat - Amount, 0.0f, MaxHeat);
	
	if (CurrentHeat != OldHeat)
	{
		OnHeatChanged.Broadcast(CurrentHeat, MaxHeat);
	}
	
	return true;
}

void UResourceManager::DissipateHeat(float DeltaTime)
{
	// Heat system only active in Forge path
	if (!IsHeatSystemActive())
	{
		return;
	}
	
	if (CurrentHeat <= 0.0f)
	{
		return;
	}
	
	float OldHeat = CurrentHeat;
	CurrentHeat = FMath::Max(0.0f, CurrentHeat - (HeatDissipationRate * DeltaTime));
	
	if (CurrentHeat != OldHeat)
	{
		OnHeatChanged.Broadcast(CurrentHeat, MaxHeat);
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
	CurrentWP = 0.0f; // Reset to 0 (good state)
	CurrentHeat = 0.0f;
	bIsOverheated = false;
	WPMaxReachedCount = 0; // Reset the counter
	
	// Clear overheat timer if active
	if (GetGameInstance() && GetGameInstance()->GetWorld())
	{
		GetGameInstance()->GetWorld()->GetTimerManager().ClearTimer(OverheatCooldownTimer);
	}
	
	// Broadcast updates
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	OnHeatChanged.Broadcast(CurrentHeat, MaxHeat);
	
	// Reset threshold
	PreviousWPThreshold = EResourceThreshold::Normal;
}

void UResourceManager::ResetWPAfterMax()
{
	CurrentWP = 0.0f;
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	
	// Reset threshold to normal
	PreviousWPThreshold = EResourceThreshold::Normal;
	CheckWPThreshold();
	
	UE_LOG(LogTemp, Warning, TEXT("ResourceManager: WP RESET TO 0 after ultimate ability use!"));
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

void UResourceManager::EndOverheatShutdown()
{
	bIsOverheated = false;
	// Optionally broadcast an event for UI to show overheat ended
	UE_LOG(LogTemp, Log, TEXT("Overheat shutdown ended"));
}

bool UResourceManager::WouldAddingWPCauseOverflow(float Amount) const
{
	// For Hacker path, we want to allow reaching 100% WP to trigger ultimate mode
	// Only block if it would go OVER 100%
	float ResultingWP = CurrentWP + Amount;
	
	// Allow abilities that would bring us exactly to 100% or less
	return ResultingWP > MaxWP;
}

bool UResourceManager::WouldAddingHeatCauseOverflow(float Amount) const
{
	// Check if adding this amount would exceed max or warning threshold
	float ResultingHeat = CurrentHeat + Amount;
	
	// Prevent abilities at 80% or higher (warning threshold)
	if (CurrentHeat >= GetHeatWarningThreshold())
	{
		return true;
	}
	
	// Check if it would exceed max
	return ResultingHeat > MaxHeat;
}