#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilityComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityComponent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float Cooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float Cost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float Range;

	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	float CurrentCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	bool bIsOnCooldown;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual bool CanExecute() const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual void Execute();

	UFUNCTION(BlueprintCallable, Category = "Ability")
	float GetCooldownRemaining() const { return CurrentCooldown; }

	UFUNCTION(BlueprintCallable, Category = "Ability")
	float GetCooldownPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool IsOnCooldown() const { return bIsOnCooldown; }

	UFUNCTION(BlueprintCallable, Category = "Ability")
	float GetRange() const { return Range; }

	UFUNCTION(BlueprintCallable, Category = "Ability")
	float GetCost() const { return Cost; }

protected:
	void StartCooldown();
	void ResetCooldown();
};