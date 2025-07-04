#include "HeatComponent.h"

UHeatComponent::UHeatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// Heat starts at 0 (no heat)
	CurrentValue = 0.0f;
	MaxValue = 100.0f;
	RegenRate = 0.0f; // We'll use DissipationRate instead
	
	// Default heat settings
	DissipationRate = 5.0f; // 5 heat per second
	WarningThreshold = 0.8f; // 80% heat
	OverheatThreshold = 1.0f; // 100% heat
}

void UHeatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Heat starts at 0
	CurrentValue = 0.0f;
}

void UHeatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Don't call parent tick as we handle dissipation differently
	// Heat dissipates over time
	if (CurrentValue > 0.0f && DissipationRate > 0.0f)
	{
		float DissipationAmount = DissipationRate * DeltaTime;
		ModifyValue(-DissipationAmount);
	}
}

void UHeatComponent::AddHeat(float Amount)
{
	if (Amount > 0.0f)
	{
		ModifyValue(Amount);
	}
}

bool UHeatComponent::IsOverheated() const
{
	return GetPercentage() >= OverheatThreshold;
}