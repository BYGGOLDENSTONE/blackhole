#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/ComboDataAsset.h"
#include "ComboDetectionSubsystem.generated.h"

USTRUCT()
struct FComboInputRecord
{
    GENERATED_BODY()

    EComboInput Input = EComboInput::None;
    float Timestamp = 0.0f;
    FVector InputLocation = FVector::ZeroVector;
};

USTRUCT()
struct FActiveComboState
{
    GENERATED_BODY()

    // Store combo by name instead of pointer to avoid dangling references
    FName ComboName;
    int32 CurrentStep = 0;
    float WindowEndTime = 0.0f;
    bool bIsActive = false;
    
    // Cache the total steps to avoid lookups
    int32 TotalSteps = 0;

    void Reset()
    {
        ComboName = NAME_None;
        CurrentStep = 0;
        WindowEndTime = 0.0f;
        bIsActive = false;
        TotalSteps = 0;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboStarted, const FComboDefinition&, Combo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComboCompleted, const FComboDefinition&, Combo, bool, bPerfectTiming);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboBroken);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComboProgress, int32, CurrentStep, int32, TotalSteps);

/**
 * Improved combo detection subsystem using data-driven combos
 * Manages input tracking and combo state for all players
 */
UCLASS()
class BLACKHOLE_API UComboDetectionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem implementation
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    // Input registration
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void RegisterInput(AActor* InputOwner, EComboInput Input, FVector InputLocation = FVector::ZeroVector);

    // State queries
    UFUNCTION(BlueprintPure, Category = "Combo")
    bool IsInCombo(AActor* Owner) const;

    UFUNCTION(BlueprintPure, Category = "Combo")
    float GetComboWindowRemaining(AActor* Owner) const;

    UFUNCTION(BlueprintPure, Category = "Combo")
    int32 GetComboProgress(AActor* Owner) const;

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void SetComboDataAsset(UComboDataAsset* DataAsset);

    // Cleanup
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void CleanupActor(AActor* Actor);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboStarted OnComboStarted;

    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboCompleted OnComboCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboBroken OnComboBroken;

    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboProgress OnComboProgress;

protected:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    UComboDataAsset* ComboData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    float InputHistoryDuration = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    float PerfectTimingWindow = 0.15f;

private:
    // Per-player combo state
    UPROPERTY()
    TMap<AActor*, FActiveComboState> ActiveCombos;
    
    // Input histories - Cannot use UPROPERTY with nested containers (TMap with TArray values)
    TMap<AActor*, TArray<FComboInputRecord>> InputHistories;

    // Update combo states
    void UpdateComboStates();
    void CheckForNewCombos(AActor* Owner, EComboInput NewInput);
    void AdvanceCombo(AActor* Owner, FActiveComboState& ComboState, EComboInput NewInput);
    void CompleteCombo(AActor* Owner, const FActiveComboState& ComboState);
    void BreakCombo(AActor* Owner);

    // Helper functions
    void CleanupInputHistory(AActor* Owner);
    bool IsInputWithinWindow(float InputTime, float WindowStart, float WindowEnd) const;
    float CalculateTimingScore(float InputTime, float IdealTime) const;

    // Timer handle for periodic updates
    FTimerHandle UpdateTimerHandle;
};