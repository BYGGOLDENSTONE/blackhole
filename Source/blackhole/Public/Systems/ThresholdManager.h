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
	int32 NextCastWPRefund = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityDisabled, UAbilityComponent*, Ability, int32, TotalDisabled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurvivorBuff, const FSurvivorBuff&, Buff);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatEnded);

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
	
private:
	// Current threshold state
	FThresholdState ThresholdState;
	
	// Current survivor buff
	FSurvivorBuff CurrentBuff;
	
	// Track if we're in combat
	bool bIsInCombat = false;
	
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
	
	// Disable random abilities based on threshold
	void DisableRandomAbilities(int32 NumberToDisable);
	
	// Update survivor buffs
	void UpdateSurvivorBuffs();
	
	// Cache player abilities
	void CachePlayerAbilities();
	
	// Helper to get random ability that isn't disabled
	UAbilityComponent* GetRandomEnabledAbility() const;
};