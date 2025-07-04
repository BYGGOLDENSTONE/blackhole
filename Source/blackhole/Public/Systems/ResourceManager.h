#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "blackhole.h"
#include "ResourceManager.generated.h"

UENUM(BlueprintType)
enum class EResourceThreshold : uint8
{
	Normal        UMETA(DisplayName = "Normal (>60%)"),
	Warning       UMETA(DisplayName = "Warning (30-60%)"),
	Critical      UMETA(DisplayName = "Critical (10-30%)"),
	Emergency     UMETA(DisplayName = "Emergency (<10%)")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceChanged, float, NewValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThresholdReached, EResourceThreshold, Threshold);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOverheatShutdown);

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
	FOnResourceChanged OnHeatChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Resource Events")
	FOnThresholdReached OnWillPowerThresholdReached;
	
	UPROPERTY(BlueprintAssignable, Category = "Resource Events")
	FOnOverheatShutdown OnOverheatShutdown;
	
	// Resource modification with validation
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool ConsumeWillPower(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddWillPower(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddHeat(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void DissipateHeat(float DeltaTime);
	
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void SetHeatDissipationRate(float NewRate) { HeatDissipationRate = NewRate; }
	
	// Getters
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetWillPowerPercent() const { return MaxWP > 0.0f ? CurrentWP / MaxWP : 0.0f; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetHeatPercent() const { return MaxHeat > 0.0f ? CurrentHeat / MaxHeat : 0.0f; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetCurrentWillPower() const { return CurrentWP; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetMaxWillPower() const { return MaxWP; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetCurrentHeat() const { return CurrentHeat; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	float GetMaxHeat() const { return MaxHeat; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool IsOverheated() const { return bIsOverheated; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	EResourceThreshold GetCurrentWPThreshold() const;
	
	// Check if player has enough WP
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool HasEnoughWillPower(float Amount) const { return CurrentWP >= Amount; }

	// Check if can consume resources
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool CanConsumeWillPower(float Amount) const { return CurrentWP >= Amount; }

	UFUNCTION(BlueprintPure, Category = "Resources")
	bool CanConsumeHeat(float Amount) const { return CurrentHeat >= Amount; }

	// Consume heat (for Forge abilities)
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool ConsumeHeat(float Amount);
	
	// Reset resources (for respawn, etc.)
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void ResetResources();
	
	// Path management
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void SetCurrentPath(ECharacterPath NewPath) { CurrentPath = NewPath; }
	
	UFUNCTION(BlueprintPure, Category = "Resources")
	ECharacterPath GetCurrentPath() const { return CurrentPath; }
	
	// Check if heat system is active (Forge path only)
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool IsHeatSystemActive() const { return CurrentPath == ECharacterPath::Forge; }
	
private:
	// Resource values
	UPROPERTY()
	float CurrentWP;
	
	UPROPERTY()
	float MaxWP;
	
	UPROPERTY()
	float CurrentHeat;
	
	UPROPERTY()
	float MaxHeat;
	
	// Heat dissipation rate per second
	UPROPERTY()
	float HeatDissipationRate;
	
	// Overheat state
	UPROPERTY()
	bool bIsOverheated;
	
	// Overheat cooldown timer
	FTimerHandle OverheatCooldownTimer;
	
	// Previous threshold for change detection
	EResourceThreshold PreviousWPThreshold;
	
	// Current character path
	UPROPERTY()
	ECharacterPath CurrentPath;
	
	// Helper functions
	void CheckWPThreshold();
	void EndOverheatShutdown();
	
	// Default values
	static constexpr float DEFAULT_MAX_WP = 100.0f;
	static constexpr float DEFAULT_MAX_HEAT = 100.0f;
	static constexpr float DEFAULT_HEAT_DISSIPATION_RATE = 5.0f;
	static constexpr float OVERHEAT_SHUTDOWN_DURATION = 3.0f;
	static constexpr float HEAT_WARNING_THRESHOLD = 0.8f;
	static constexpr float HEAT_OVERHEAT_THRESHOLD = 1.0f;
};