#pragma once

#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "KillAbilityComponent.generated.h"

class UWillPowerComponent;
class UIntegrityComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UKillAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UKillAbilityComponent();

	virtual void Execute() override;
	virtual bool CanExecute() const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UWillPowerComponent* WillPowerComponent;
};