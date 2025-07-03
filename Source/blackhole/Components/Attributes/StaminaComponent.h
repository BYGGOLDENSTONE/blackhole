#pragma once

#include "CoreMinimal.h"
#include "AttributeComponent.h"
#include "StaminaComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UStaminaComponent : public UAttributeComponent
{
	GENERATED_BODY()

public:
	UStaminaComponent();

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool UseStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool HasEnoughStamina(float Amount) const;

protected:
	virtual void BeginPlay() override;
};