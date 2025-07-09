#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HitStopManager.generated.h"

USTRUCT(BlueprintType)
struct FHitStopConfig
{
	GENERATED_BODY()
	
	// Duration of the hit stop in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = 0.1f;
	
	// Time dilation factor (0.1 = 10% speed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeDilation = 0.1f;
	
	// Whether this is a critical hit (longer duration)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCritical = false;
	
	// Priority (higher priority hit stops override lower ones)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 0;
	
	FHitStopConfig()
	{
		Duration = 0.1f;
		TimeDilation = 0.1f;
		bIsCritical = false;
		Priority = 0;
	}
	
	FHitStopConfig(float InDuration, float InTimeDilation, bool bInCritical = false, int32 InPriority = 0)
		: Duration(InDuration)
		, TimeDilation(InTimeDilation)
		, bIsCritical(bInCritical)
		, Priority(InPriority)
	{}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitStopStarted, const FHitStopConfig&, Config);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHitStopEnded);

/**
 * Manages hit stop effects for impactful combat feedback
 */
UCLASS()
class BLACKHOLE_API UHitStopManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Subsystem implementation
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	
	// Request a hit stop effect
	UFUNCTION(BlueprintCallable, Category = "HitStop")
	void RequestHitStop(const FHitStopConfig& Config);
	
	// Quick methods for common hit stops
	UFUNCTION(BlueprintCallable, Category = "HitStop")
	void RequestLightHitStop() { RequestHitStop(FHitStopConfig(0.05f, 0.3f, false, 1)); }
	
	UFUNCTION(BlueprintCallable, Category = "HitStop")
	void RequestMediumHitStop() { RequestHitStop(FHitStopConfig(0.1f, 0.1f, false, 2)); }
	
	UFUNCTION(BlueprintCallable, Category = "HitStop")
	void RequestHeavyHitStop() { RequestHitStop(FHitStopConfig(0.15f, 0.05f, false, 3)); }
	
	UFUNCTION(BlueprintCallable, Category = "HitStop")
	void RequestCriticalHitStop() { RequestHitStop(FHitStopConfig(0.25f, 0.01f, true, 5)); }
	
	// Check if hit stop is active
	UFUNCTION(BlueprintPure, Category = "HitStop")
	bool IsHitStopActive() const { return bIsActive; }
	
	// Get current hit stop config
	UFUNCTION(BlueprintPure, Category = "HitStop")
	FHitStopConfig GetCurrentConfig() const { return CurrentConfig; }
	
	// Force end hit stop
	UFUNCTION(BlueprintCallable, Category = "HitStop")
	void EndHitStop();
	
	// Events
	UPROPERTY(BlueprintAssignable, Category = "HitStop")
	FOnHitStopStarted OnHitStopStarted;
	
	UPROPERTY(BlueprintAssignable, Category = "HitStop")
	FOnHitStopEnded OnHitStopEnded;

protected:
	// Apply time dilation
	void ApplyTimeDilation(float Dilation);
	
	// Restore normal time
	void RestoreTimeDilation();

private:
	// Current hit stop state
	bool bIsActive = false;
	FHitStopConfig CurrentConfig;
	float RemainingDuration = 0.0f;
	
	// Timer handle for ending hit stop
	FTimerHandle HitStopTimerHandle;
	
	// Original time dilation before hit stop
	float OriginalTimeDilation = 1.0f;
	
	// Configuration
	UPROPERTY(EditDefaultsOnly, Category = "HitStop")
	float MinDuration = 0.016f; // Minimum 1 frame at 60fps
	
	UPROPERTY(EditDefaultsOnly, Category = "HitStop")
	float MaxDuration = 0.5f; // Maximum half second
	
	UPROPERTY(EditDefaultsOnly, Category = "HitStop")
	bool bAllowStacking = false; // If true, hit stops can stack duration
};