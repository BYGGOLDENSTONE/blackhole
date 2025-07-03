#pragma once

#include "CoreMinimal.h"
#include "AttributeComponent.h"
#include "WillPowerComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UWillPowerComponent : public UAttributeComponent
{
	GENERATED_BODY()

public:
	UWillPowerComponent();

	UFUNCTION(BlueprintCallable, Category = "WillPower")
	bool UseWillPower(float Amount);

	UFUNCTION(BlueprintCallable, Category = "WillPower")
	bool HasEnoughWillPower(float Amount) const;

	UFUNCTION(BlueprintCallable, Category = "WillPower")
	void DrainWillPower(float Amount);

protected:
	virtual void BeginPlay() override;
};