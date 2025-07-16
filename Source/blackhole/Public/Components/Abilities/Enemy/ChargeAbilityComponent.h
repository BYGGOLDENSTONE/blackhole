#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "ChargeAbilityComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UChargeAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UChargeAbilityComponent();

	// Charge stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge", meta = (DisplayName = "Charge Speed"))
	float ChargeSpeed = 1200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge", meta = (DisplayName = "Charge Distance"))
	float ChargeDistance = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge", meta = (DisplayName = "Impact Damage"))
	float ImpactDamage = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge", meta = (DisplayName = "Impact Radius"))
	float ImpactRadius = 200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge", meta = (DisplayName = "Knockback Force"))
	float KnockbackForce = 800.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge", meta = (DisplayName = "Min Charge Distance"))
	float MinChargeDistance = 300.0f;

	virtual void Execute() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintPure, Category = "Charge")
	bool IsCharging() const { return bIsCharging; }

protected:
	virtual void BeginPlay() override;

private:
	bool bIsCharging;
	FVector ChargeDirection;
	FVector ChargeStartLocation;
	float ChargeTimeElapsed;
	
	UPROPERTY()
	AActor* ChargeTarget;
	
	void StartCharge();
	void UpdateCharge(float DeltaTime);
	void EndCharge(bool bHitTarget);
	void ApplyImpactDamage(const FVector& ImpactLocation);
};