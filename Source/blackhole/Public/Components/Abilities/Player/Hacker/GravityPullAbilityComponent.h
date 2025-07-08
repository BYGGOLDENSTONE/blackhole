#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "GravityPullAbilityComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UGravityPullAbilityComponent : public UAbilityComponent
{
    GENERATED_BODY()

public:
    UGravityPullAbilityComponent();

    virtual void Execute() override;
    virtual bool CanExecute() const override;
    virtual void ExecuteUltimate() override;

protected:
    virtual void BeginPlay() override;

public:
    // Launch force multiplier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float LaunchForceMultiplier = 10.0f;

    // Base launch force
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float BaseLaunchForce = 1000.0f;

    // Maximum targeting angle (in degrees) from camera forward
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float MaxTargetingAngle = 45.0f;

private:
    // Find the best hackable target
    class AActor* FindBestHackableTarget() const;

    // Get camera location and direction
    void GetCameraLocationAndDirection(FVector& OutLocation, FVector& OutDirection) const;

    // Check if target is within angle threshold
    bool IsWithinTargetingAngle(const FVector& CameraDirection, const FVector& ToTarget) const;
};