#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ComboSystem.generated.h"

UENUM(BlueprintType)
enum class EComboInputType : uint8
{
    None,
    Slash,
    Dash,
    Jump,
    FirewallBreach,
    PulseHack,
    GravityPull,
    MoltenMace,
    HeatShield,
    BlastCharge,
    HammerStrike
};

USTRUCT(BlueprintType)
struct FComboInput
{
    GENERATED_BODY()

    UPROPERTY()
    EComboInputType InputType = EComboInputType::None;

    UPROPERTY()
    float Timestamp = 0.0f;

    UPROPERTY()
    FVector InputLocation = FVector::ZeroVector;

    FComboInput() {}
    FComboInput(EComboInputType Type) : InputType(Type) 
    {
        Timestamp = FPlatformTime::Seconds();
    }
};

USTRUCT(BlueprintType)
struct FComboPattern
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ComboName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EComboInputType> RequiredInputs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<float> TimingWindows;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ResourceDiscount = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DamageMultiplier = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag ComboTag;

    // Visual effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UParticleSystem* ComboEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USoundBase* ComboSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UCameraShakeBase> CameraShake;
};

USTRUCT(BlueprintType)
struct FActiveCombo
{
    GENERATED_BODY()

    UPROPERTY()
    FComboPattern Pattern;

    UPROPERTY()
    int32 CurrentInputIndex = 0;

    UPROPERTY()
    float WindowTimeRemaining = 0.0f;

    UPROPERTY()
    bool bPerfectTiming = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComboExecuted, const FComboPattern&, Combo, bool, bPerfectTiming);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboWindowOpen, float, WindowDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboWindowClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComboProgress, int32, CurrentHits, int32, TotalHits);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UComboSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UComboSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Input registration
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void RegisterInput(EComboInputType InputType, FVector InputLocation = FVector::ZeroVector);

    // Combo execution
    UFUNCTION(BlueprintCallable, Category = "Combo")
    bool TryExecuteCombo(const FComboPattern& ComboPattern);

    // State queries
    UFUNCTION(BlueprintPure, Category = "Combo")
    bool IsComboWindowOpen() const { return ActiveCombo.WindowTimeRemaining > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Combo")
    float GetComboWindowRemaining() const { return ActiveCombo.WindowTimeRemaining; }

    UFUNCTION(BlueprintPure, Category = "Combo")
    bool IsInCombo() const { return ActiveCombo.CurrentInputIndex > 0; }

    UFUNCTION(BlueprintPure, Category = "Combo")
    int32 GetCurrentComboLength() const { return ActiveCombo.CurrentInputIndex; }

    // Get active combo for checking state
    const FActiveCombo& GetActiveCombo() const { return ActiveCombo; }

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void RegisterComboPattern(const FComboPattern& Pattern);

    UFUNCTION(BlueprintCallable, Category = "Combo")
    void ClearComboPatterns();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboExecuted OnComboExecuted;

    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboWindowOpen OnComboWindowOpen;

    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboWindowClosed OnComboWindowClosed;

    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboProgress OnComboProgress;

protected:
    // Registered combo patterns
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TArray<FComboPattern> RegisteredCombos;

    // Input history
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combo|Debug")
    TArray<FComboInput> InputHistory;

    // Current active combo
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combo|Debug")
    FActiveCombo ActiveCombo;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo|Config")
    float InputHistoryDuration = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo|Config")
    float PerfectTimingWindow = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo|Config")
    float ComboResetDelay = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo|Config")
    bool bDebugDrawComboWindow = false;

private:
    // Helper functions
    void UpdateInputHistory(float DeltaTime);
    bool CheckForComboMatch();
    void ResetCombo();
    void StartComboWindow(float Duration);
    void CloseComboWindow();
    bool IsInputWithinWindow(const FComboInput& Input, float WindowStart, float WindowEnd) const;
    float CalculateTimingScore(const FComboInput& Input, float IdealTime) const;

    // Combo execution helpers
    void ExecuteComboEffects(const FComboPattern& Combo, bool bPerfectTiming);
    void ApplyResourceDiscount(const FComboPattern& Combo);
    void ApplyDamageBonus(const FComboPattern& Combo);

    // Visual feedback
    void PlayComboFeedback(const FComboPattern& Combo);
    void ShowComboUI(const FComboPattern& Combo, bool bPerfectTiming);

    // Cached references
    UPROPERTY()
    class ABlackholePlayerCharacter* OwnerCharacter;

    UPROPERTY()
    class UResourceManager* ResourceManager;

    // Timing
    float LastInputTime = 0.0f;
    float ComboResetTimer = 0.0f;
    
    // Register default combo patterns
    void RegisterDefaultCombos();
};