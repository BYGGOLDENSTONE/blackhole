#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
#include "MoltenMaceSlashAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UMoltenMaceSlashAbility : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UMoltenMaceSlashAbility();

	virtual void Execute() override;
	virtual bool CanExecute() const override;

protected:
	// Ability parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float SlashDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float SlashRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float SlashAngle = 120.0f; // Cone angle in degrees

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float StaggerDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float BurnDamagePerSecond = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float BurnDuration = 3.0f;

	// Visual effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* SlashEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* BurnEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* SlashSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* ImpactSound;

private:
	void ApplyBurnDamage(class ABaseEnemy* Enemy);
	void DealBurnTick(class ABaseEnemy* Enemy, int32 TicksRemaining);
	bool IsInSlashCone(const FVector& TargetLocation, const FVector& CharacterLocation, const FVector& CharacterForward) const;
};