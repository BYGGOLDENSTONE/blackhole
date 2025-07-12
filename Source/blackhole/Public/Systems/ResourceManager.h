#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "blackhole.h"
#include "Config/GameplayConfig.h"
#include "ResourceManager.generated.h"

UENUM(BlueprintType)
enum class EResourceThreshold : uint8
{
	Normal        UMETA(DisplayName = "Normal (0-50%)"),
	Buffed        UMETA(DisplayName = "Buffed (50-100%)"),
	Critical      UMETA(DisplayName = "Critical (100%)"),
	// Legacy values kept for compatibility
	Warning       UMETA(DisplayName = "Warning (Legacy)"),
	Emergency     UMETA(DisplayName = "Emergency (Legacy)")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceChanged, float, NewValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThresholdReached, EResourceThreshold, Threshold);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWPMaxReached, int32, TimesReached);

UCLASS()
class BLACKHOLE_API UResourceManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// Initialization
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Delegates for UI binding
	UPROPERTY(BlueprintAssignable, Category = "Resource Events")
	FOnResourceChanged OnWillPowerChanged;
		
	UPROPERTY(BlueprintAssignable, Category = "Resource Events")
	FOnThresholdReached OnWillPowerThresholdReached;
	
	UPROPERTY(BlueprintAssignable, Category = "Resource Events")
	FOnWPMaxReached OnWPMaxReached;
		
	// Resource modification with validation
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool ConsumeWillPower(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddWillPower(float Amount);
		
	// Getters
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetWillPowerPercent() const { return MaxWP > 0.0f ? CurrentWP / MaxWP : 0.0f; }
		
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetCurrentWillPower() const { return CurrentWP; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetMaxWillPower() const { return MaxWP; }
		
	UFUNCTION(BlueprintPure, Category = "Resources")
	EResourceThreshold GetCurrentWPThreshold() const;
	
	// Check if player has enough WP
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool HasEnoughWillPower(float Amount) const { return CurrentWP >= Amount; }

	// Check if can consume resources
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool CanConsumeWillPower(float Amount) const { return CurrentWP >= Amount; }
	
	// Check if adding resources would cause overflow
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool WouldAddingWPCauseOverflow(float Amount) const;
		
	// Get warning thresholds
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetWPWarningThreshold() const { return MaxWP * GameplayConfig::Resources::WP_WARNING_PERCENT; } // 90%
		
	// Reset resources (for respawn, etc.)
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void ResetResources();
	
	// Path management
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void SetCurrentPath(ECharacterPath NewPath) { CurrentPath = NewPath; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	ECharacterPath GetCurrentPath() const { return CurrentPath; }
	
	
	// Get how many times WP has reached 100%
	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetWPMaxReachedCount() const { return WPMaxReachedCount; }
	
	// Reset WP to 0 (called after reaching 100%)
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void ResetWPAfterMax();
	
	// Set flag to allow WP reset (must be called before ResetWPAfterMax)
	void AuthorizeWPReset() { bWPResetAuthorized = true; }
	
	// Critical state management (prevents resets during critical timer)
	void SetCriticalState(bool bCritical) { bInCriticalState = bCritical; }
	bool IsInCriticalState() const { return bInCriticalState; }
	
private:
	// Resource values
	UPROPERTY()
	float CurrentWP;
	
	UPROPERTY()
	float MaxWP;
		
	// Previous threshold for change detection
	EResourceThreshold PreviousWPThreshold;
	
	// Current character path
	UPROPERTY()
	ECharacterPath CurrentPath;
	
	// Track how many times WP has reached 100%
	UPROPERTY()
	int32 WPMaxReachedCount;
	
	// Authorization flag for WP reset (prevents unwanted resets)
	bool bWPResetAuthorized = false;
	
	// Flag to prevent resets during critical gameplay moments
	bool bInCriticalState = false;
	
	// Helper functions
	void CheckWPThreshold();
	void SyncWillPowerComponent(); // Keep player's WillPowerComponent in sync with ResourceManager
	
	// Default values now in GameplayConfig.h
};