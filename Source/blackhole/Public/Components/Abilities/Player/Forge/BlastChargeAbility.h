#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
#include "BlastChargeAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UBlastChargeAbility : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UBlastChargeAbility();

	virtual void Execute() override;
	virtual bool CanExecute() const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Ability parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float BlastRadius = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float BlastDamage = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float KnockbackForce = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float EnvironmentDamage = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float ChargeTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bCanDestroyEnvironment = true;

	// Visual effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* ChargeEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* BlastEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* ChargeSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* BlastSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TSubclassOf<class UCameraShakeBase> BlastCameraShake;

private:
	UPROPERTY()
	bool bIsCharging;

	UPROPERTY()
	class UParticleSystemComponent* ActiveChargeEffect;

	FTimerHandle ChargeTimerHandle;

	void ExecuteBlast();
	void ApplyKnockback(AActor* Target, const FVector& BlastOrigin);
	void DamageEnvironment(const FVector& BlastOrigin);
};