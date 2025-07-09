#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
#include "Config/GameplayConfig.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "SystemOverrideAbility.generated.h"

class ABaseEnemy;

USTRUCT(BlueprintType)
struct FSystemDisable
{
	GENERATED_BODY()

	UPROPERTY()
	float Duration = 0.0f;

	UPROPERTY()
	float OriginalMaxWalkSpeed = 0.0f;

	FTimerHandle RestoreTimerHandle;

	FSystemDisable()
	{
		Duration = GameplayConfig::Abilities::SystemOverride::DISABLE_DURATION;
		OriginalMaxWalkSpeed = 0.0f;
	}

	FSystemDisable(float InDuration, float InOriginalSpeed)
		: Duration(InDuration), OriginalMaxWalkSpeed(InOriginalSpeed)
	{
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API USystemOverrideAbility : public UAbilityComponent
{
	GENERATED_BODY()

public:
	USystemOverrideAbility();

	virtual void Execute() override;
	virtual bool CanExecute() const override;
	virtual void ExecuteUltimate() override;

	// Clean up disable effects on EndPlay
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Ability parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float DisableDuration = GameplayConfig::Abilities::SystemOverride::DISABLE_DURATION;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float EffectRadius = GameplayConfig::Abilities::SystemOverride::RADIUS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float WPCleanse = GameplayConfig::Abilities::SystemOverride::WP_CLEANSE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float AreaDamage = GameplayConfig::Abilities::SystemOverride::DAMAGE;

	// Visual and audio effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* OverrideEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* DisableEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* ActivationSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* DisableSound;

private:
	// Disable tracking - Use weak pointers to prevent crashes when enemies are destroyed
	TMap<TWeakObjectPtr<ABaseEnemy>, FSystemDisable> DisabledEnemies;

	// Core functionality
	void PerformSystemOverride();
	void DisableEnemy(ABaseEnemy* Enemy);
	void RestoreEnemy(ABaseEnemy* Enemy);

	// Helper functions
	TArray<ABaseEnemy*> GetEnemiesInRadius() const;
};