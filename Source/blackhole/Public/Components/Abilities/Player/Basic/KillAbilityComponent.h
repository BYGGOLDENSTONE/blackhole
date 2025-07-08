#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "KillAbilityComponent.generated.h"

class UIntegrityComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UKillAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UKillAbilityComponent();

	virtual void Execute() override;
	virtual void ExecuteUltimate() override;
	virtual bool CanExecute() const override;

protected:
	virtual void BeginPlay() override;
};