#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

// Delegate for when attribute value changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeValueChanged, float, NewValue, float, OldValue);

// Delegate for when attribute reaches zero
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttributeReachedZero);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float CurrentValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float RegenRate;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	virtual void ModifyValue(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetCurrentValue() const { return CurrentValue; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxValue() const { return MaxValue; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void SetCurrentValue(float NewValue);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void SetMaxValue(float NewValue);
	
	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnAttributeValueChanged OnValueChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnAttributeReachedZero OnReachedZero;
};