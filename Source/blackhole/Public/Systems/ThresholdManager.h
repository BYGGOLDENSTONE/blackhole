#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ThresholdManager.generated.h"

class UAbilityComponent;
class ABlackholePlayerCharacter;
class UResourceManager;

USTRUCT(BlueprintType)
struct FThresholdState
{
	GENERATED_BODY()
	
	UPROPERTY()
	bool bHighTriggered = false;
	
	UPROPERTY()
	bool bMediumTriggered = false;
	
	UPROPERTY()
	bool bLowTriggered = false;
	
	UPROPERTY()
	bool bCriticalTriggered = false;
	
	void Reset() { *this = FThresholdState(); }
};

USTRUCT(BlueprintType)
struct FSurvivorBuff
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	float DamageMultiplier = 1.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float CooldownReduction = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float AttackSpeed = 1.0f;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsBuffed = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityDisabled, UAbilityComponent*, Ability, int32, TotalDisabled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurvivorBuff, const FSurvivorBuff&, Buff);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUltimateModeActivated, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCriticalTimer, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCriticalTimerExpired);

UCLASS()
class BLACKHOLE_API UThresholdManager : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// Subsystem overrides
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Events
	UPROPERTY(BlueprintAssignable, Category = "Threshold Events")
	FOnAbilityDisabled OnAbilityDisabled;
	
	UPROPERTY(BlueprintAssignable, Category = "Threshold Events")
	FOnSurvivorBuff OnSurvivorBuff;
	
	UPROPERTY(BlueprintAssignable, Category = "Threshold Events")
	FOnCombatStarted OnCombatStarted;
	
	UPROPERTY(BlueprintAssignable, Category = "Threshold Events")
	FOnCombatEnded OnCombatEnded;
	
	UPROPERTY(BlueprintAssignable, Category = "Threshold Events")
	FOnPlayerDeath OnPlayerDeath;
	
	UPROPERTY(BlueprintAssignable, Category = "Threshold Events")
	FOnUltimateModeActivated OnUltimateModeActivated;
	
	UPROPERTY(BlueprintAssignable, Category = "Threshold Events")
	FOnCriticalTimer OnCriticalTimer;
	
	UPROPERTY(BlueprintAssignable, Category = "Threshold Events")
	FOnCriticalTimerExpired OnCriticalTimerExpired;
	
	// Start/End combat tracking
	UFUNCTION(BlueprintCallable, Category = "Threshold")
	void StartCombat();
	
	UFUNCTION(BlueprintCallable, Category = "Threshold")
	void EndCombat();
	
	// Get current survivor buff
	UFUNCTION(BlueprintPure, Category = "Threshold")
	const FSurvivorBuff& GetCurrentBuff() const { return CurrentBuff; }
	
	// Check if ability is disabled
	UFUNCTION(BlueprintPure, Category = "Threshold")
	bool IsAbilityDisabled(UAbilityComponent* Ability) const;
	
	// Get disabled ability count
	UFUNCTION(BlueprintPure, Category = "Threshold")
	int32 GetDisabledAbilityCount() const { return DisabledAbilities.Num(); }
	
	// Check if ultimate mode is active
	UFUNCTION(BlueprintPure, Category = "Threshold")
	bool IsUltimateModeActive() const { return bUltimateModeActive; }
	
	// Check if in combat
	UFUNCTION(BlueprintPure, Category = "Threshold")
	bool IsInCombat() const { return bIsInCombat; }
	
	// Check if critical timer is active
	UFUNCTION(BlueprintPure, Category = "Threshold")
	bool IsCriticalTimerActive() const;
	
	// Get critical timer remaining time
	UFUNCTION(BlueprintPure, Category = "Threshold")
	float GetCriticalTimeRemaining() const;
	
	// Called when any ability is executed
	void OnAbilityExecuted(UAbilityComponent* Ability);
	
	// Force ultimate mode activation (for testing)
	void ActivateUltimateMode();
	
	// Start critical timer when WP reaches 0
	void StartCriticalTimer();
	
	// Force cache player abilities (for testing)
	void CachePlayerAbilities();
	
	// Get random enabled ability
	UAbilityComponent* GetRandomEnabledAbility() const;
	UAbilityComponent* GetRandomEnabledAbilityExcludingSlash() const;
	
private:
	// Clean up invalid ability references
	void CleanupInvalidAbilities();
	// Current threshold state
	FThresholdState ThresholdState;
	
	// Current survivor buff
	FSurvivorBuff CurrentBuff;
	
	// Track if we're in combat
	bool bIsInCombat = false;
	
	// Track if ultimate mode is active
	bool bUltimateModeActive = false;
	
	// Track if we're currently processing ultimate deactivation
	bool bIsDeactivatingUltimate = false;
	
	// Time when ultimate mode was activated
	float UltimateModeActivationTime = 0.0f;
	
	// Critical timer variables
	FTimerHandle CriticalTimerHandle;
	FTimerHandle CriticalUpdateTimerHandle;
	float CriticalTimerDuration = 5.0f; // 5 seconds
	float CriticalTimerStartTime = 0.0f;
	bool bCriticalTimerActive = false;
	
	// Disabled abilities
	UPROPERTY()
	TArray<UAbilityComponent*> DisabledAbilities;
	
	// All player abilities (cached)
	UPROPERTY()
	TArray<UAbilityComponent*> AllPlayerAbilities;
	
	// Cached references
	UPROPERTY()
	ABlackholePlayerCharacter* PlayerCharacter;
	
	UPROPERTY()
	UResourceManager* ResourceManager;
	
	// Handle WP threshold changes
	UFUNCTION()
	void OnWPThresholdChanged(EResourceThreshold NewThreshold);
	
	// Deactivate ultimate mode and disable the used ability
	void DeactivateUltimateMode(UAbilityComponent* UsedAbility);
	
	// Update survivor buffs
	void UpdateSurvivorBuffs();
	
	// Critical timer functions
	void StopCriticalTimer();
	void OnCriticalTimerExpiredInternal(); // Internal function for timer callback
	void UpdateCriticalTimer();
};