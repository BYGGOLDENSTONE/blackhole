#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DeathManager.generated.h"

UENUM(BlueprintType)
enum class EDeathReason : uint8
{
    None,
    HealthDepleted,
    TooManyAbilitiesLost,
    WPOverload,
    Environmental,
    Debug
};

USTRUCT(BlueprintType)
struct FDeathConditions
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxAbilityLosses = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxWPOverloads = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bInstantDeathOnMaxAbilities = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAllowRevive = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDeathWithReason, EDeathReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathConditionWarning, EDeathReason, PotentialReason);

/**
 * Manages player death conditions and triggers
 * Separated from ThresholdManager for clarity
 */
UCLASS()
class BLACKHOLE_API UDeathManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem implementation
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Death condition checks
    UFUNCTION(BlueprintCallable, Category = "Death")
    void CheckDeathConditions();

    // Death triggers
    UFUNCTION(BlueprintCallable, Category = "Death")
    void TriggerDeath(EDeathReason Reason);

    // State tracking
    UFUNCTION(BlueprintCallable, Category = "Death")
    void IncrementAbilityLosses();

    UFUNCTION(BlueprintCallable, Category = "Death")
    void IncrementWPOverloads();

    UFUNCTION(BlueprintCallable, Category = "Death")
    void OnHealthDepleted();

    // State queries
    UFUNCTION(BlueprintPure, Category = "Death")
    bool IsPlayerDead() const { return bIsPlayerDead; }

    UFUNCTION(BlueprintPure, Category = "Death")
    int32 GetAbilitiesLost() const { return AbilitiesLost; }

    UFUNCTION(BlueprintPure, Category = "Death")
    int32 GetWPOverloadCount() const { return WPOverloadCount; }

    UFUNCTION(BlueprintPure, Category = "Death")
    EDeathReason GetDeathReason() const { return LastDeathReason; }

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "Death")
    void SetDeathConditions(const FDeathConditions& Conditions);

    // Reset
    UFUNCTION(BlueprintCallable, Category = "Death")
    void ResetDeathState();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Death")
    FOnPlayerDeathWithReason OnPlayerDeath;

    UPROPERTY(BlueprintAssignable, Category = "Death")
    FOnDeathConditionWarning OnDeathConditionWarning;

protected:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death")
    FDeathConditions DeathConditions;

    // State tracking
    UPROPERTY()
    bool bIsPlayerDead = false;

    UPROPERTY()
    int32 AbilitiesLost = 0;

    UPROPERTY()
    int32 WPOverloadCount = 0;

    UPROPERTY()
    EDeathReason LastDeathReason = EDeathReason::None;

    // Cached references
    UPROPERTY()
    class ABlackholePlayerCharacter* PlayerCharacter;

private:
    // Check specific death conditions
    bool CheckHealthDeath() const;
    bool CheckAbilityLossDeath() const;
    bool CheckWPOverloadDeath() const;

    // Handle actual death
    void ExecuteDeath(EDeathReason Reason);
};