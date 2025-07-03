#pragma once

#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "SystemFreezeAbilityComponent.generated.h"

class UWillPowerComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API USystemFreezeAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	USystemFreezeAbilityComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Freeze")
	float StunDuration;

	virtual void Execute() override;
	virtual bool CanExecute() const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UWillPowerComponent* WillPowerComponent;
};