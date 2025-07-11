#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
#include "Config/GameplayConfig.h"
#include "PulseHackAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UPulseHackAbility : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UPulseHackAbility();

	virtual void Execute() override;
	virtual bool CanExecute() const override;
	virtual void ExecuteUltimate() override;

protected:
	// Ability parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float HackRadius = GameplayConfig::Abilities::PulseHack::RADIUS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float SlowDuration = GameplayConfig::Abilities::PulseHack::SLOW_DURATION;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float SlowAmount = GameplayConfig::Abilities::PulseHack::SLOW_AMOUNT; // 70% slow

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float WPRefundPerEnemy = GameplayConfig::Abilities::PulseHack::WP_REFUND_PER_ENEMY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float MaxWPRefund = GameplayConfig::Abilities::PulseHack::MAX_WP_REFUND;

	// Ultimate version parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Pulse Hack", meta = (ClampMin = "1.0", ClampMax = "5.0"))
	float UltimateRadiusMultiplier = GameplayConfig::Abilities::PulseHack::ULTIMATE_RADIUS_MULT;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Pulse Hack", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float UltimateStunDuration = GameplayConfig::Abilities::PulseHack::ULTIMATE_STUN_DURATION;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Pulse Hack", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float UltimateWPCleanse = GameplayConfig::Abilities::PulseHack::ULTIMATE_WP_CLEANSE;

	// Visual effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* PulseEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* PulseSound;

private:
	void ApplySlowToEnemy(class ABaseEnemy* Enemy);
	void RestoreEnemySpeed(class ABaseEnemy* Enemy, float OriginalSpeed);
};