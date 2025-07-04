#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
#include "HammerStrikeAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UHammerStrikeAbility : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UHammerStrikeAbility();

	virtual void Execute() override;
	virtual bool CanExecute() const override;

protected:
	// Ability parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float StrikeDamage = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float StrikeRange = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float StunDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float ComboWindowDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float ComboDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	int32 MaxComboHits = 3;

	// Visual effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* StrikeEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* StunEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* StrikeSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* StunSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TSubclassOf<class UCameraShakeBase> StrikeCameraShake;

private:
	UPROPERTY()
	bool bComboWindowActive;

	UPROPERTY()
	int32 CurrentComboCount;

	UPROPERTY()
	TArray<class ABaseEnemy*> StunnedEnemies;

	FTimerHandle ComboWindowTimer;

	void EndComboWindow();
	void StunEnemy(class ABaseEnemy* Enemy);
	void RemoveStun(class ABaseEnemy* Enemy);
	float GetComboDamage() const;
};