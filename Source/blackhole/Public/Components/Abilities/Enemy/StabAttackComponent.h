#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/Enemy/EnemyAbilityComponent.h"
#include "StabAttackComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UStabAttackComponent : public UEnemyAbilityComponent
{
	GENERATED_BODY()

public:
	UStabAttackComponent();

	// Basic attack stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stab Attack", meta = (DisplayName = "Base Damage"))
	float BaseDamage = 15.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stab Attack", meta = (DisplayName = "Attack Range"))
	float AttackRange = 150.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stab Attack", meta = (DisplayName = "Attack Angle", ClampMin = "10.0", ClampMax = "90.0"))
	float AttackAngle = 45.0f; // Cone angle for hit detection
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stab Attack", meta = (DisplayName = "Attack Speed", ClampMin = "0.1", ClampMax = "3.0"))
	float AttackSpeed = 1.5f; // Animation speed multiplier

	virtual void Execute() override;
	
	// Getters for state machine access
	UFUNCTION(BlueprintPure, Category = "Stab Attack")
	float GetBaseDamage() const { return BaseDamage; }
	
	UFUNCTION(BlueprintCallable, Category = "Stab Attack")
	void SetBaseDamage(float NewDamage) { BaseDamage = NewDamage; }

protected:
	virtual void BeginPlay() override;

private:
	void PerformStabAttack();
};