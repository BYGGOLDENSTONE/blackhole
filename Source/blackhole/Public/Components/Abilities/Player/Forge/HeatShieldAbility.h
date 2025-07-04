#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
#include "HeatShieldAbility.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldBroken, float, RemainingDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldDamageAbsorbed, float, DamageAbsorbed);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UHeatShieldAbility : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UHeatShieldAbility();

	virtual void Execute() override;
	virtual bool CanExecute() const override;

	// Called by IntegrityComponent to check shield
	UFUNCTION(BlueprintCallable, Category = "Shield")
	float AbsorbDamage(float IncomingDamage);

	UFUNCTION(BlueprintPure, Category = "Shield")
	bool IsShieldActive() const { return bShieldActive; }

	UFUNCTION(BlueprintPure, Category = "Shield")
	float GetShieldHealth() const { return CurrentShieldHealth; }

	UFUNCTION(BlueprintPure, Category = "Shield")
	float GetShieldHealthPercent() const;

	// Events
	UPROPERTY(BlueprintAssignable)
	FOnShieldBroken OnShieldBroken;

	UPROPERTY(BlueprintAssignable)
	FOnShieldDamageAbsorbed OnShieldDamageAbsorbed;

protected:
	// Shield parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float MaxShieldHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float ShieldDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float DamageReflectPercent = 0.2f; // 20% damage reflection

	// Visual effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* ShieldEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* ShieldBreakEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* ShieldActivateSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* ShieldHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* ShieldBreakSound;

private:
	UPROPERTY()
	bool bShieldActive;

	UPROPERTY()
	float CurrentShieldHealth;

	UPROPERTY()
	class UParticleSystemComponent* ActiveShieldEffect;

	FTimerHandle ShieldDurationTimer;

	void DeactivateShield();
	void BreakShield(float RemainingDamage);
};