#include "StaminaComponent.h"

UStaminaComponent::UStaminaComponent()
{
	MaxValue = 100.0f;
	CurrentValue = 100.0f;
	RegenRate = 10.0f;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UStaminaComponent::UseStamina(float Amount)
{
	if (HasEnoughStamina(Amount))
	{
		ModifyValue(-Amount);
		return true;
	}
	return false;
}

bool UStaminaComponent::HasEnoughStamina(float Amount) const
{
	return CurrentValue >= Amount;
}