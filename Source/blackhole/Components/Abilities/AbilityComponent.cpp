#include "AbilityComponent.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	Cooldown = 1.0f;
	Cost = 0.0f;
	Range = 1000.0f;
	CurrentCooldown = 0.0f;
	bIsOnCooldown = false;
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentCooldown = 0.0f;
	bIsOnCooldown = false;
}

void UAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsOnCooldown && CurrentCooldown > 0.0f)
	{
		CurrentCooldown -= DeltaTime;
		
		if (CurrentCooldown <= 0.0f)
		{
			ResetCooldown();
		}
	}
}

bool UAbilityComponent::CanExecute() const
{
	return !bIsOnCooldown;
}

void UAbilityComponent::Execute()
{
	if (CanExecute())
	{
		StartCooldown();
	}
}

float UAbilityComponent::GetCooldownPercentage() const
{
	return Cooldown > 0.0f ? CurrentCooldown / Cooldown : 0.0f;
}

void UAbilityComponent::StartCooldown()
{
	bIsOnCooldown = true;
	CurrentCooldown = Cooldown;
}

void UAbilityComponent::ResetCooldown()
{
	bIsOnCooldown = false;
	CurrentCooldown = 0.0f;
}