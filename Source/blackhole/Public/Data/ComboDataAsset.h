#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ComboDataAsset.generated.h"

UENUM(BlueprintType)
enum class EComboInput : uint8
{
    None,
    Slash,
    Dash,
    Jump,
    WallRun,
    Ability1,
    Ability2,
    Ability3,
    Ability4
};

USTRUCT(BlueprintType)
struct FComboStep
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EComboInput RequiredInput = EComboInput::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float TimeWindow = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AnimationTag;
};

USTRUCT(BlueprintType)
struct FComboDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ComboName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FComboStep> Steps;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<class UComboAbilityComponent> ComboAbilityClass;

    // Resource modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ResourceDiscount = 0.25f;

    // Damage modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", ClampMax = "5.0"))
    float DamageMultiplier = 1.5f;

    // Visual/Audio
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UParticleSystem* ComboEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USoundBase* ComboSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UCameraShakeBase> CameraShake;

    // UI
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* ComboIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor ComboColor = FLinearColor::White;
};

/**
 * Data asset containing all combo definitions for the game
 * Allows designers to create and modify combos without code changes
 */
UCLASS(BlueprintType)
class BLACKHOLE_API UComboDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combos")
    TArray<FComboDefinition> ComboDefinitions;

    /**
     * Find a combo definition by name
     * @param ComboName The name to search for
     * @param OutCombo The found combo definition (if any)
     * @return True if combo was found
     */
    UFUNCTION(BlueprintCallable, Category = "Combos")
    bool FindComboByName(FName ComboName, FComboDefinition& OutCombo) const;

    /**
     * Get all combos that start with a specific input
     */
    UFUNCTION(BlueprintCallable, Category = "Combos")
    TArray<FComboDefinition> GetCombosStartingWith(EComboInput Input) const;

    /**
     * Validate all combo definitions (called in editor)
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#if WITH_EDITOR
    /**
     * Editor-only validation
     */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};