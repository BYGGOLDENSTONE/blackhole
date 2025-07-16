#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "SwordAttackComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API USwordAttackComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	USwordAttackComponent();

	// Sword attack stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sword Attack", meta = (DisplayName = "Base Damage"))
	float BaseDamage = 20.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sword Attack", meta = (DisplayName = "Attack Range"))
	float AttackRange = 180.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sword Attack", meta = (DisplayName = "Attack Angle", ClampMin = "10.0", ClampMax = "90.0"))
	float AttackAngle = 60.0f; // Wider cone than stab
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sword Attack", meta = (DisplayName = "Attack Speed", ClampMin = "0.1", ClampMax = "3.0"))
	float AttackSpeed = 1.0f; // Standard speed

	virtual void Execute() override;
	
	// Getters for state machine access
	UFUNCTION(BlueprintPure, Category = "Sword Attack")
	float GetBaseDamage() const { return BaseDamage; }
	
	UFUNCTION(BlueprintCallable, Category = "Sword Attack")
	void SetBaseDamage(float NewDamage) { BaseDamage = NewDamage; }

protected:
	virtual void BeginPlay() override;

private:
	void PerformSwordAttack();
};