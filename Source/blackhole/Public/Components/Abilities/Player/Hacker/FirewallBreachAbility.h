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
	
	virtual void Execute() override;
	virtual bool CanExecute() const override;
	
protected:
	virtual void BeginPlay() override;
};