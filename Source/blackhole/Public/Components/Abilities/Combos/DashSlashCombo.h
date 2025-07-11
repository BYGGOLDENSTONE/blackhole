#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/ComboAbilityComponent.h"
#include "DashSlashCombo.generated.h"

/**
 * Dash + Slash combo - Phantom Strike
 * Teleports behind target for critical backstab
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UDashSlashCombo : public UComboAbilityComponent
{
    GENERATED_BODY()

public:
    UDashSlashCombo();

    // Phantom Strike specific parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike", meta = (ClampMin = "50", ClampMax = "500"))
    float TeleportDistance = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike", meta = (ClampMin = "1.0", ClampMax = "3.0"))
    float BackstabDamageMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike")
    bool bAutoRotateToTarget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PhantomAfterImageDuration = 0.5f;

    // Aim forgiveness for easier combo execution
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike", meta = (ClampMin = "10.0", ClampMax = "300.0"))
    float AimForgivenessRadius = 180.0f;

    // Visual effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike|Visual")
    UParticleSystem* TeleportParticle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike|Visual")
    USoundBase* TeleportSound = nullptr;

    virtual void ExecuteCombo() override;

private:
    bool TeleportBehindTarget(AActor* Target);
    void CreatePhantomAfterImage();
    AActor* FindBestTarget(const FVector& Start, const FVector& Forward, float SearchRange);
};