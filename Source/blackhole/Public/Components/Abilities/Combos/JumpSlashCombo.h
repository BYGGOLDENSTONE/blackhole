#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/ComboAbilityComponent.h"
#include "JumpSlashCombo.generated.h"

/**
 * Jump + Slash combo - Aerial Rave
 * Downward slash with shockwave on landing
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UJumpSlashCombo : public UComboAbilityComponent
{
    GENERATED_BODY()

public:
    UJumpSlashCombo();

    // Aerial Rave specific parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave", meta = (ClampMin = "100", ClampMax = "1000"))
    float ShockwaveRadius = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave", meta = (ClampMin = "10", ClampMax = "200"))
    float ShockwaveDamage = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float ShockwaveDelay = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave")
    bool bRequireAirborne = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave", meta = (ClampMin = "0.0", ClampMax = "2000.0"))
    float DownwardForce = 1000.0f;

    // Aim forgiveness for easier combo execution
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave", meta = (ClampMin = "10.0", ClampMax = "300.0"))
    float AimForgivenessRadius = 200.0f;

    // Visual effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave|Visual")
    UParticleSystem* ShockwaveParticle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave|Visual")
    USoundBase* ShockwaveSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave|Visual")
    FLinearColor ShockwaveColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);

    virtual void ExecuteCombo() override;

private:
    void CreateShockwave();
    void ApplyDownwardSlash();
    bool IsCharacterAirborne() const;
    AActor* FindBestTarget(const FVector& Start, const FVector& Forward, float SearchRange);

    FTimerHandle ShockwaveTimerHandle;
};