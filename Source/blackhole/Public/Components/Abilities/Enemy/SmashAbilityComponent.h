#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "SmashAbilityComponent.generated.h"

class UIntegrityComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API USmashAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	USmashAbilityComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smash")
	float Damage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smash")
	bool bIsAreaDamage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smash", meta = (EditCondition = "bIsAreaDamage"))
	float AreaRadius;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smash", meta = (EditCondition = "bIsAreaDamage"))
	float KnockbackForce;

	virtual void Execute() override;
	
	// Getters and setters
	UFUNCTION(BlueprintPure, Category = "Smash")
	float GetDamage() const { return Damage; }
	
	UFUNCTION(BlueprintCallable, Category = "Smash")
	void SetDamage(float NewDamage) { Damage = NewDamage; }
	
	UFUNCTION(BlueprintCallable, Category = "Smash")
	void SetAreaDamage(bool bArea) { bIsAreaDamage = bArea; }
	
	UFUNCTION(BlueprintCallable, Category = "Smash")
	void SetAreaRadius(float NewRadius) { AreaRadius = NewRadius; }
	
	UFUNCTION(BlueprintPure, Category = "Smash")
	float GetAreaRadius() const { return AreaRadius; }
	
	// For attack speed modifications
	UFUNCTION(BlueprintCallable, Category = "Smash")
	void SetCooldownTime(float NewCooldown) { Cooldown = NewCooldown; }
	
	UFUNCTION(BlueprintPure, Category = "Smash")
	float GetCooldownTime() const { return Cooldown; }
	
	UFUNCTION(BlueprintCallable, Category = "Smash")
	void SetKnockbackForce(float NewForce) { KnockbackForce = NewForce; }
	
	UFUNCTION(BlueprintPure, Category = "Smash")
	float GetKnockbackForce() const { return KnockbackForce; }

protected:
	virtual void BeginPlay() override;
	
	void PerformSingleTargetDamage(AActor* Owner);
	void PerformAreaDamage(AActor* Owner);
};