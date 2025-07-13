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

	virtual void Execute() override;
	
	// Getters and setters
	UFUNCTION(BlueprintPure, Category = "Smash")
	float GetDamage() const { return Damage; }
	
	UFUNCTION(BlueprintCallable, Category = "Smash")
	void SetDamage(float NewDamage) { Damage = NewDamage; }
	
	UFUNCTION(BlueprintCallable, Category = "Smash")
	void SetAreaDamage(bool bArea) { bIsAreaDamage = bArea; }

protected:
	virtual void BeginPlay() override;
	
	void PerformSingleTargetDamage(AActor* Owner);
	void PerformAreaDamage(AActor* Owner);
};