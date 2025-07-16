// AreaDamageAbilityComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "AreaDamageAbilityComponent.generated.h"

UENUM(BlueprintType)
enum class EAreaDamagePattern : uint8
{
    Circular      UMETA(DisplayName = "Circular (Default)"),
    Cone          UMETA(DisplayName = "Cone"),
    Line          UMETA(DisplayName = "Line"),
    Cross         UMETA(DisplayName = "Cross Pattern")
};

USTRUCT(BlueprintType)
struct FAreaDamageVisualEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    UParticleSystem* ImpactParticle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    USoundBase* ImpactSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSubclassOf<UCameraShakeBase> CameraShake = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    float CameraShakeRadius = 1000.0f;
};

/**
 * Configurable area damage ability that can be added to any enemy
 * Supports various patterns, damage falloff, knockback, and post-damage effects
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UAreaDamageAbilityComponent : public UAbilityComponent
{
    GENERATED_BODY()

public:
    UAreaDamageAbilityComponent();

    // Area Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (DisplayName = "Damage Pattern"))
    EAreaDamagePattern DamagePattern = EAreaDamagePattern::Circular;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (ClampMin = "50.0", ClampMax = "5000.0", DisplayName = "Damage Radius"))
    float DamageRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (DisplayName = "Cone Angle (Degrees)", ClampMin = "15.0", ClampMax = "360.0"))
    float ConeAngle = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (DisplayName = "Line Width", ClampMin = "10.0", ClampMax = "1000.0"))
    float LineWidth = 150.0f;

    // Damage Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage")
    float BaseDamage = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (DisplayName = "Enable Damage Falloff"))
    bool bUseDamageFalloff = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (DisplayName = "Min Damage Percent", ClampMin = "0.0", ClampMax = "1.0"))
    float MinDamagePercent = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (DisplayName = "Damage Player WP"))
    bool bDamagePlayerWP = true;

    // Knockback Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback")
    bool bApplyKnockback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback", meta = (ClampMin = "0.0", ClampMax = "10000.0", EditCondition = "bApplyKnockback", DisplayName = "Knockback Force"))
    float KnockbackForce = 750.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback", meta = (DisplayName = "Knockback Up Ratio", ClampMin = "0.0", ClampMax = "2.0"))
    float KnockbackUpRatio = 0.3f;

    // Post-Damage Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Effects")
    bool bApplyStagger = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Effects", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float StaggerDuration = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Effects")
    bool bApplySlow = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Effects", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float SlowDuration = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Effects", meta = (DisplayName = "Slow Percent", ClampMin = "0.0", ClampMax = "1.0"))
    float SlowPercent = 0.5f;

    // Visual/Audio Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    FAreaDamageVisualEffect VisualEffects;

    // Targeting
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
    bool bDamageEnemies = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
    bool bDamagePlayers = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
    bool bDamageSelf = false;

    // Animation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* PreDamageMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (DisplayName = "Pre-Damage Delay", ClampMin = "0.0", ClampMax = "5.0"))
    float PreDamageDelay = 0.3f;
    
    // Additional Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General", meta = (DisplayName = "Custom Ability Name"))
    FString CustomAbilityName = "Area Damage";
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General", meta = (DisplayName = "Debug Visualization"))
    bool bShowDebugVisualization = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (DisplayName = "Hit Prevention Duration", ClampMin = "0.0", ClampMax = "2.0"))
    float HitPreventionDuration = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Damage", meta = (DisplayName = "Damage Type Class"))
    TSubclassOf<UDamageType> DamageTypeClass = nullptr;

public:
    virtual void Execute() override;
    virtual void ExecuteUltimate() override;

private:
    void PerformAreaDamage();
    TArray<AActor*> GetActorsInPattern();
    bool IsActorInCone(AActor* Actor, const FVector& ConeOrigin, const FVector& ConeDirection) const;
    bool IsActorInLine(AActor* Actor, const FVector& LineStart, const FVector& LineDirection) const;
    bool IsActorInCross(AActor* Actor, const FVector& CrossCenter) const;
    float CalculateDamageFalloff(float Distance) const;
    void ApplyDamageToActor(AActor* Target, float FinalDamage);
    void ApplyKnockbackToActor(AActor* Target);
    void ApplyPostEffectsToActor(AActor* Target);
    void PlayVisualEffects();
    bool HasRecentlyHitActor(AActor* Actor) const;
    void TrackHitActor(AActor* Actor);

    FTimerHandle PreDamageTimerHandle;
    FVector DamageOrigin;
    FVector DamageDirection;
    
    // Track recently hit actors to prevent multiple hits
    TMap<AActor*, float> RecentlyHitActors;
};