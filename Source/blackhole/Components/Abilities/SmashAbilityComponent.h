#pragma once

#include "CoreMinimal.h"
#include "AbilityComponent.h"
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

	virtual void Execute() override;

protected:
	virtual void BeginPlay() override;
};