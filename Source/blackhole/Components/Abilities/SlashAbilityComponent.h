#pragma once

#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "SlashAbilityComponent.generated.h"

class UStaminaComponent;
class UIntegrityComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API USlashAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	USlashAbilityComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash")
	float Damage;

	virtual void Execute() override;
	virtual bool CanExecute() const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UStaminaComponent* StaminaComponent;
};