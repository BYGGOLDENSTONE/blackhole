#pragma once

#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "DodgeComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UDodgeComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UDodgeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	float DodgeDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	float DodgeDuration;

	virtual void Execute() override;

protected:
	virtual void BeginPlay() override;
};