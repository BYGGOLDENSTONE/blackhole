#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "FirewallBreachAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UFirewallBreachAbility : public UAbilityComponent
{
	GENERATED_BODY()
	
public:
	UFirewallBreachAbility();
	
	// Armor shred parameters
	UPROPERTY(EditDefaultsOnly, Category = "Firewall Breach")
	float ArmorReductionPercent = 0.5f; // 50% armor reduction
	
	UPROPERTY(EditDefaultsOnly, Category = "Firewall Breach")
	float EffectDuration = 5.0f;
	
	// Ultimate version parameters
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate Firewall Breach", meta = (ClampMin = "1.0", ClampMax = "5.0"))
	float UltimateRadiusMultiplier = 2.0f; // Multiply base range by this for ultimate
	
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate Firewall Breach", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UltimateArmorReduction = 1.0f; // 100% armor reduction
	
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate Firewall Breach", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float UltimateDuration = 10.0f;
	
	virtual void Execute() override;
	virtual bool CanExecute() const override;
	virtual void ExecuteUltimate() override;
	
protected:
	virtual void BeginPlay() override;
};