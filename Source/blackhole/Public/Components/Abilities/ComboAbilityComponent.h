#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "ComboAbilityComponent.generated.h"

/**
 * Base class for combo abilities with editable parameters
 */
UCLASS(Abstract, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UComboAbilityComponent : public UAbilityComponent
{
    GENERATED_BODY()

public:
    UComboAbilityComponent();

    // Combo timing parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Timing", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float ComboWindowTime = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Timing", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float TimeSlowScale = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Timing", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float TimeSlowDuration = 0.15f;

    // Combat parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Combat", meta = (ClampMin = "10", ClampMax = "200"))
    float Damage = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Combat", meta = (ClampMin = "1.0", ClampMax = "5.0"))
    float DamageMultiplier = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Combat", meta = (ClampMin = "100", ClampMax = "2000"))
    float ComboRange = 300.0f;

    // Visual feedback
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Visual")
    bool bShowDebugVisuals = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Visual")
    FLinearColor ComboTrailColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);

    // Sound/Haptic feedback
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Feedback")
    USoundBase* ComboSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Feedback")
    UParticleSystem* ComboParticle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Feedback", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float CameraShakeScale = 1.0f;

    // Override Execute to prevent direct execution - combos are triggered differently
    virtual void Execute() override;
    virtual bool CanExecute() const override;

protected:
    // Pure virtual function that derived classes must implement
    virtual void ExecuteCombo() PURE_VIRTUAL(UComboAbilityComponent::ExecuteCombo,);

    // Helper functions for combo execution
    void ApplyTimeSlow();
    void ApplyHitStop(class UHitStopManager* HitStopMgr, float DamageAmount);
    void DrawComboVisuals(const FVector& Start, const FVector& End);
    void PlayComboFeedback(const FVector& Location);

    // Cached references
    UPROPERTY()
    class ABlackholePlayerCharacter* OwnerCharacter = nullptr;

    UPROPERTY()
    class UWorld* CachedWorld = nullptr;

    // Time slow tracking
    float TimeSlowEndTime = 0.0f;
    bool bIsTimeSlowActive = false;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};