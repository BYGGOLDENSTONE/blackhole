#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "HeatAuraComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UHeatAuraComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UHeatAuraComponent();

	// Heat aura stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Aura", meta = (DisplayName = "Damage Per Second"))
	float DamagePerSecond = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Aura", meta = (DisplayName = "Aura Radius"))
	float AuraRadius = 300.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Aura", meta = (DisplayName = "Tick Interval"))
	float TickInterval = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Aura", meta = (DisplayName = "Affects Enemies"))
	bool bAffectsEnemies = true;

	virtual void Execute() override;
	virtual void Deactivate() override;
	
	// Toggle aura on/off
	UFUNCTION(BlueprintCallable, Category = "Heat Aura")
	void SetAuraActive(bool bActive);
	
	UFUNCTION(BlueprintPure, Category = "Heat Aura")
	bool IsAuraActive() const { return bAuraActive; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle AuraTickHandle;
	bool bAuraActive;
	
	void AuraTick();
	void ApplyAuraDamage();
};