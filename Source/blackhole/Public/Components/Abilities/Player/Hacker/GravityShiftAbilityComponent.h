#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Components/GravityDirectionComponent.h"
#include "GravityShiftAbilityComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UGravityShiftAbilityComponent : public UAbilityComponent
{
    GENERATED_BODY()

public:
    UGravityShiftAbilityComponent();

    virtual void Execute() override;
    virtual bool CanExecute() const override;

protected:
    virtual void BeginPlay() override;

public:
    // Gravity shift parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    float ShiftRadius = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    float TransitionDuration = 4.0f;  // Slower, smoother transition

    // Should cycle through gravity axes or use look direction?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    bool bUseLookDirection = false;


private:
    // Get next gravity axis in cycle
    EGravityAxis GetNextGravityAxis() const;

    // Get gravity direction from look
    FVector GetGravityFromLookDirection() const;

    // Apply gravity shift to area
    void ApplyAreaGravityShift(FVector NewGravityDirection, float Radius, float Duration);

    // Cached reference
    UPROPERTY()
    class UGravityDirectionComponent* GravityComponent;

    // Current axis index for cycling
    mutable int32 CurrentAxisIndex = 0;
};