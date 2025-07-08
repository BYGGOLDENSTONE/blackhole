#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
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
	float HackRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float SlowDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float SlowAmount = 0.3f; // 70% slow

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float WPRefundPerEnemy = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float MaxWPRefund = 15.0f;

	// Visual effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* PulseEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* PulseSound;

private:
	void ApplySlowToEnemy(class ABaseEnemy* Enemy);
	void RestoreEnemySpeed(class ABaseEnemy* Enemy, float OriginalSpeed);
};