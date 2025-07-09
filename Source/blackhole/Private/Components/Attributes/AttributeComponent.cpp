#include "Components/Attributes/AttributeComponent.h"
#include "Config/GameplayConfig.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	MaxValue = GameplayConfig::Attributes::DEFAULT_MAX_VALUE;
	CurrentValue = GameplayConfig::Attributes::DEFAULT_MAX_VALUE;
	RegenRate = GameplayConfig::Attributes::DEFAULT_REGEN_RATE;
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
	float OldValue = CurrentValue;
	CurrentValue = FMath::Clamp(CurrentValue + Amount, 0.0f, MaxValue);
	
	// Broadcast value change if it actually changed
	if (!FMath::IsNearlyEqual(OldValue, CurrentValue))
	{
		OnValueChanged.Broadcast(CurrentValue, OldValue);
		
		// Check if we just reached zero
		if (CurrentValue <= 0.0f && OldValue > 0.0f)
		{
			OnReachedZero.Broadcast();
		}
	}
}

float UAttributeComponent::GetPercentage() const
{
	return MaxValue > 0.0f ? CurrentValue / MaxValue : 0.0f;
}

void UAttributeComponent::SetCurrentValue(float NewValue)
{
	float OldValue = CurrentValue;
	CurrentValue = FMath::Clamp(NewValue, 0.0f, MaxValue);
	
	// Broadcast value change if it actually changed
	if (!FMath::IsNearlyEqual(OldValue, CurrentValue))
	{
		OnValueChanged.Broadcast(CurrentValue, OldValue);
		
		// Check if we just reached zero
		if (CurrentValue <= 0.0f && OldValue > 0.0f)
		{
			OnReachedZero.Broadcast();
		}
	}
}

void UAttributeComponent::SetMaxValue(float NewValue)
{
	MaxValue = FMath::Max(0.0f, NewValue);
	CurrentValue = FMath::Min(CurrentValue, MaxValue);
}