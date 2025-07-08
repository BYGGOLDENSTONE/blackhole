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
	
	// Called when any ability is executed
	void OnAbilityExecuted(UAbilityComponent* Ability);
	
	// Force ultimate mode activation (for testing)
	void ActivateUltimateMode();
	
	// Force cache player abilities (for testing)
	void CachePlayerAbilities();
	
private:
	// Current threshold state
	FThresholdState ThresholdState;
	
	// Current survivor buff
	FSurvivorBuff CurrentBuff;
	
	// Track if we're in combat
	bool bIsInCombat = false;
	
	// Track if ultimate mode is active
	bool bUltimateModeActive = false;
	
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
	
	// Handle WP reaching 100%
	UFUNCTION()
	void OnWPMaxReachedHandler(int32 TimesReached);
	
	// Deactivate ultimate mode and disable the used ability
	void DeactivateUltimateMode(UAbilityComponent* UsedAbility);
	
	// Update survivor buffs
	void UpdateSurvivorBuffs();
	
	// Helper to get random ability that isn't disabled
	UAbilityComponent* GetRandomEnabledAbility() const;
	
	// Helper to get random ability excluding slash
	UAbilityComponent* GetRandomEnabledAbilityExcludingSlash() const;
};