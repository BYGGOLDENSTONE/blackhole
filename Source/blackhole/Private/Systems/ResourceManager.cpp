#include "Systems/ResourceManager.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UResourceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Initialize default values
	MaxWP = DEFAULT_MAX_WP;
	CurrentWP = 0.0f; // WP starts at 0, gained from hacker enemies
	MaxHeat = DEFAULT_MAX_HEAT;
	CurrentHeat = 0.0f;
	HeatDissipationRate = DEFAULT_HEAT_DISSIPATION_RATE;
	bIsOverheated = false;
	PreviousWPThreshold = EResourceThreshold::Normal; // Starting at 0% WP = Normal state
	CurrentPath = ECharacterPath::Hacker; // Default to Hacker
}

void UResourceManager::Deinitialize()
{
	// Clear any active timers
	if (GetGameInstance() && GetGameInstance()->GetWorld())
	{
		GetGameInstance()->GetWorld()->GetTimerManager().ClearTimer(OverheatCooldownTimer);
	}
	
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
		return false;
	}
	
	// Check if we're overheated (abilities disabled) - only in Forge path
	if (IsHeatSystemActive() && bIsOverheated)
	{
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
	if (Amount < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddWillPower called with negative amount: %f"), Amount);
		return;
	}
	
	float OldWP = CurrentWP;
	CurrentWP = FMath::Clamp(CurrentWP + Amount, 0.0f, MaxWP);
	
	if (CurrentWP != OldWP)
	{
		OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
		CheckWPThreshold();
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
		if (!bIsOverheated && GetHeatPercent() >= HEAT_OVERHEAT_THRESHOLD)
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
					OVERHEAT_SHUTDOWN_DURATION,
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
	
	// Inverted system: Higher WP = More ability loss
	if (Percent <= 0.1f)
	{
		return EResourceThreshold::Normal; // 0-10% WP = All abilities available
	}
	else if (Percent <= 0.3f)
	{
		return EResourceThreshold::Warning; // 10-30% WP = Lose 1 ability
	}
	else if (Percent <= 0.6f)
	{
		return EResourceThreshold::Critical; // 30-60% WP = Lose 2 abilities
	}
	else
	{
		return EResourceThreshold::Emergency; // >60% WP = Lose 3 abilities
	}
}

void UResourceManager::ResetResources()
{
	CurrentWP = 0.0f; // Reset to 0 (good state)
	CurrentHeat = 0.0f;
	bIsOverheated = false;
	
	// Clear overheat timer if active
	if (GetGameInstance() && GetGameInstance()->GetWorld())
	{
		GetGameInstance()->GetWorld()->GetTimerManager().ClearTimer(OverheatCooldownTimer);
	}
	
	// Broadcast updates
	OnWillPowerChanged.Broadcast(CurrentWP, MaxWP);
	OnHeatChanged.Broadcast(CurrentHeat, MaxHeat);
	
	// Reset threshold (Normal = 0-10% WP in new system)
	PreviousWPThreshold = EResourceThreshold::Normal;
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