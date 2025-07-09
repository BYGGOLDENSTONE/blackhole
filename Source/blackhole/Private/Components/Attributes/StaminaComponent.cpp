#include "Components/Attributes/StaminaComponent.h"
#include "Config/GameplayConfig.h"

UStaminaComponent::UStaminaComponent()
{
	MaxValue = GameplayConfig::Attributes::Stamina::MAX_VALUE;
	CurrentValue = GameplayConfig::Attributes::Stamina::MAX_VALUE;
	RegenRate = GameplayConfig::Attributes::Stamina::REGEN_RATE;
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