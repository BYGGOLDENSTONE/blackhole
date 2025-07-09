#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
#include "Config/GameplayConfig.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DataSpikeAbility.generated.h"

class ABaseEnemy;

USTRUCT(BlueprintType)
struct FDataCorruption
{
	GENERATED_BODY()

	UPROPERTY()
	float DamagePerTick = 0.0f;

	UPROPERTY()
	float Duration = 0.0f;

	UPROPERTY()
	float TickRate = 0.5f;

	FTimerHandle DOTTimerHandle;
	int32 TicksRemaining = 0;

	FDataCorruption()
	{
		DamagePerTick = GameplayConfig::Abilities::DataSpike::DOT_DAMAGE;
		Duration = GameplayConfig::Abilities::DataSpike::DOT_DURATION;
		TickRate = GameplayConfig::Abilities::DataSpike::DOT_TICK_RATE;
		TicksRemaining = FMath::CeilToInt(Duration / TickRate);
	}

	FDataCorruption(float InDamage, float InDuration, float InTickRate)
		: DamagePerTick(InDamage), Duration(InDuration), TickRate(InTickRate)
	{
		TicksRemaining = FMath::CeilToInt(Duration / TickRate);
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UDataSpikeAbility : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UDataSpikeAbility();

	virtual void Execute() override;
	virtual bool CanExecute() const override;
	virtual void ExecuteUltimate() override;

	// Clean up DOT effects on EndPlay
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Ability parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float ProjectileDamage = GameplayConfig::Abilities::DataSpike::DAMAGE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float ProjectileSpeed = GameplayConfig::Abilities::DataSpike::PROJECTILE_SPEED;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float ProjectileRange = GameplayConfig::Abilities::DataSpike::RANGE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	int32 PierceCount = GameplayConfig::Abilities::DataSpike::PIERCE_COUNT;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float DOTDamage = GameplayConfig::Abilities::DataSpike::DOT_DAMAGE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float DOTDuration = GameplayConfig::Abilities::DataSpike::DOT_DURATION;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float DOTTickRate = GameplayConfig::Abilities::DataSpike::DOT_TICK_RATE;

	// Visual and audio effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* ProjectileEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* HitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* DOTEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* HitSound;

private:
	// DOT tracking - Use weak pointers to prevent crashes when enemies are destroyed
	TMap<TWeakObjectPtr<ABaseEnemy>, FDataCorruption> ActiveDOTs;

	// Core functionality
	void FireProjectile(bool bIsUltimate = false);
	void ProcessProjectileHit(const FHitResult& HitResult, int32& RemainingPierces, bool bIsUltimate);
	void ApplyDataCorruption(ABaseEnemy* Enemy, bool bIsUltimate = false);
	void OnDOTTick(ABaseEnemy* Enemy);
	void RemoveDOTFromEnemy(ABaseEnemy* Enemy);

	// Helper functions
	FVector GetProjectileDirection() const;
	FVector GetProjectileStart() const;
};