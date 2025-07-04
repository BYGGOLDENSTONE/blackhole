#include "AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	MaxValue = 100.0f;
	CurrentValue = 100.0f;
	RegenRate = 0.0f;
}

void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentValue = MaxValue;
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (RegenRate > 0.0f && CurrentValue < MaxValue)
	{
		ModifyValue(RegenRate * DeltaTime);
	}
}

void UAttributeComponent::ModifyValue(float Amount)
{
	CurrentValue = FMath::Clamp(CurrentValue + Amount, 0.0f, MaxValue);
}

float UAttributeComponent::GetPercentage() const
{
	return MaxValue > 0.0f ? CurrentValue / MaxValue : 0.0f;
}

void UAttributeComponent::SetCurrentValue(float NewValue)
{
	CurrentValue = FMath::Clamp(NewValue, 0.0f, MaxValue);
}

void UAttributeComponent::SetMaxValue(float NewValue)
{
	MaxValue = FMath::Max(0.0f, NewValue);
	CurrentValue = FMath::Min(CurrentValue, MaxValue);
}