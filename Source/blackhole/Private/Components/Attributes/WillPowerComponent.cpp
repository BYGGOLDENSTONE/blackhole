#include "WillPowerComponent.h"

UWillPowerComponent::UWillPowerComponent()
{
	MaxValue = 100.0f;
	CurrentValue = 100.0f;
	RegenRate = 0.0f;
}

void UWillPowerComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UWillPowerComponent::UseWillPower(float Amount)
{
	if (HasEnoughWillPower(Amount))
	{
		ModifyValue(-Amount);
		return true;
	}
	return false;
}

bool UWillPowerComponent::HasEnoughWillPower(float Amount) const
{
	return CurrentValue >= Amount;
}

void UWillPowerComponent::DrainWillPower(float Amount)
{
	ModifyValue(-FMath::Abs(Amount));
}