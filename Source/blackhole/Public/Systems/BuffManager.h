#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Config/GameplayConfig.h"
#include "BuffManager.generated.h"

USTRUCT(BlueprintType)
struct FCombatBuff
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float DamageMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    float CooldownReduction = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AttackSpeedMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    float MovementSpeedMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    FString BuffSource;

    void Reset()
    {
        DamageMultiplier = 1.0f;
        CooldownReduction = 0.0f;
        AttackSpeedMultiplier = 1.0f;
        MovementSpeedMultiplier = 1.0f;
        BuffSource.Empty();
    }

    // Combine with another buff (multiplicative for multipliers, additive for reductions)
    void CombineWith(const FCombatBuff& Other)
    {
        DamageMultiplier *= Other.DamageMultiplier;
        CooldownReduction = FMath::Min(CooldownReduction + Other.CooldownReduction, 0.9f); // Cap at 90% CDR
        AttackSpeedMultiplier *= Other.AttackSpeedMultiplier;
        MovementSpeedMultiplier *= Other.MovementSpeedMultiplier;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffsChanged, const FCombatBuff&, CurrentBuffs);

/**
 * Manages combat buffs from various sources (WP thresholds, ability losses, etc.)
 * Separated from ThresholdManager for cleaner architecture
 */
UCLASS()
class BLACKHOLE_API UBuffManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem implementation
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Buff management
    UFUNCTION(BlueprintCallable, Category = "Buffs")
    void AddBuff(const FString& BuffID, const FCombatBuff& Buff);

    UFUNCTION(BlueprintCallable, Category = "Buffs")
    void RemoveBuff(const FString& BuffID);

    UFUNCTION(BlueprintCallable, Category = "Buffs")
    void ClearAllBuffs();

    // Get combined buffs
    UFUNCTION(BlueprintPure, Category = "Buffs")
    const FCombatBuff& GetCombinedBuffs() const { return CombinedBuffs; }

    // Specific buff calculations
    UFUNCTION(BlueprintPure, Category = "Buffs")
    float CalculateFinalDamage(float BaseDamage) const;

    UFUNCTION(BlueprintPure, Category = "Buffs")
    float CalculateFinalCooldown(float BaseCooldown) const;

    UFUNCTION(BlueprintPure, Category = "Buffs")
    float GetAttackSpeedMultiplier() const { return CombinedBuffs.AttackSpeedMultiplier; }

    UFUNCTION(BlueprintPure, Category = "Buffs")
    float GetMovementSpeedMultiplier() const { return CombinedBuffs.MovementSpeedMultiplier; }

    // WP threshold buffs
    UFUNCTION(BlueprintCallable, Category = "Buffs")
    void UpdateWPThresholdBuffs(float WPPercentage);

    // Lost ability buffs
    UFUNCTION(BlueprintCallable, Category = "Buffs")
    void UpdateLostAbilityBuffs(int32 AbilitiesLost);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Buffs")
    FOnBuffsChanged OnBuffsChanged;

protected:
    // Active buffs
    UPROPERTY()
    TMap<FString, FCombatBuff> ActiveBuffs;

    // Combined result
    UPROPERTY()
    FCombatBuff CombinedBuffs;

    // Recalculate combined buffs
    void RecalculateBuffs();

    // Create buff for WP threshold
    FCombatBuff CreateWPThresholdBuff(float WPPercentage) const;

    // Create buff for lost abilities
    FCombatBuff CreateLostAbilityBuff(int32 AbilitiesLost) const;
};