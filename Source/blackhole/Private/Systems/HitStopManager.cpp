#include "Systems/HitStopManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/Engine.h"

void UHitStopManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	bIsActive = false;
	RemainingDuration = 0.0f;
	OriginalTimeDilation = 1.0f;
}

void UHitStopManager::Deinitialize()
{
	// Ensure we restore time dilation on shutdown
	if (bIsActive)
	{
		EndHitStop();
	}
	
	Super::Deinitialize();
}


bool UHitStopManager::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	// Only support game worlds
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UHitStopManager::RequestHitStop(const FHitStopConfig& Config)
{
	// Safety check - don't process if world is invalid
	if (!IsValid(GetWorld()))
	{
		return;
	}
	
	// Validate config
	FHitStopConfig ValidatedConfig = Config;
	ValidatedConfig.Duration = FMath::Clamp(ValidatedConfig.Duration, MinDuration, MaxDuration);
	ValidatedConfig.TimeDilation = FMath::Clamp(ValidatedConfig.TimeDilation, 0.001f, 1.0f);
	
	// Check if we should apply this hit stop
	if (bIsActive)
	{
		// If current hit stop has higher priority, ignore this request
		if (CurrentConfig.Priority > ValidatedConfig.Priority)
		{
			return;
		}
		
		// If stacking is allowed and priorities match, add duration
		if (bAllowStacking && CurrentConfig.Priority == ValidatedConfig.Priority)
		{
			RemainingDuration += ValidatedConfig.Duration;
			RemainingDuration = FMath::Min(RemainingDuration, MaxDuration);
			return;
		}
		
		// Otherwise, override with new hit stop
		EndHitStop();
	}
	
	// Apply the hit stop
	bIsActive = true;
	CurrentConfig = ValidatedConfig;
	RemainingDuration = ValidatedConfig.Duration;
	
	// Apply time dilation
	ApplyTimeDilation(CurrentConfig.TimeDilation);
	
	// Set timer to end hit stop
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(HitStopTimerHandle, this, &UHitStopManager::EndHitStop, 
			ValidatedConfig.Duration, false);
	}
	
	// Broadcast event
	OnHitStopStarted.Broadcast(CurrentConfig);
	
	// Visual feedback
	if (CurrentConfig.bIsCritical && GEngine)
	{
		// Debug message removed - critical hit
	}
}

void UHitStopManager::EndHitStop()
{
	if (!bIsActive)
	{
		return;
	}
	
	bIsActive = false;
	RemainingDuration = 0.0f;
	
	// Restore time dilation
	RestoreTimeDilation();
	
	// Clear timer if any
	if (IsValid(GetWorld()))
	{
		GetWorld()->GetTimerManager().ClearTimer(HitStopTimerHandle);
	}
	
	// Broadcast event
	OnHitStopEnded.Broadcast();
}

void UHitStopManager::ApplyTimeDilation(float Dilation)
{
	UWorld* World = GetWorld();
	if (!World || !World->GetWorldSettings())
	{
		return;
	}
	
	// Store original time dilation
	OriginalTimeDilation = World->GetWorldSettings()->TimeDilation;
	
	// Apply new time dilation
	World->GetWorldSettings()->TimeDilation = Dilation;
	
	// Also apply to global time dilation for consistent behavior
	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		WorldSettings->SetTimeDilation(Dilation);
	}
}

void UHitStopManager::RestoreTimeDilation()
{
	UWorld* World = GetWorld();
	if (!World || !World->GetWorldSettings())
	{
		return;
	}
	
	// Restore original time dilation
	World->GetWorldSettings()->TimeDilation = OriginalTimeDilation;
	
	// Also restore global time dilation
	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		WorldSettings->SetTimeDilation(OriginalTimeDilation);
	}
}