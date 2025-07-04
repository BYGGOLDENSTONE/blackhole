#include "Components/Abilities/AbilityComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	Cooldown = 1.0f;
	Cost = 0.0f; // Legacy field
	StaminaCost = 0.0f;
	WPCost = 0.0f;
	HeatCost = 0.0f;
	Range = 1000.0f;
	HeatGenerationMultiplier = 0.5f; // Default: generates heat equal to 50% of cost
	CurrentCooldown = 0.0f;
	bIsOnCooldown = false;
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentCooldown = 0.0f;
	bIsOnCooldown = false;
	
	// Cache resource manager reference
	ResourceManager = GetResourceManager();
	
	// Cache threshold manager
	if (UWorld* World = GetWorld())
	{
		ThresholdManager = World->GetSubsystem<UThresholdManager>();
	}
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
	if (bIsOnCooldown)
	{
		return false;
	}

	// Check resource availability
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		// Check stamina availability
		if (StaminaCost > 0.0f)
		{
			// For now, assume stamina is always available - can be extended later
			// if (!ResMgr->CanConsumeStamina(StaminaCost)) return false;
		}

		// WP system is inverted: Hacker abilities ADD WP (corruption)
		// No need to check WP availability since it's added, not consumed
		// However, we might want to prevent going over 100% in the future
		
		// Check Heat availability for Forge abilities
		if (HeatCost > 0.0f && !ResMgr->CanConsumeHeat(HeatCost))
		{
			return false;
		}
	}

	return true;
}

void UAbilityComponent::Execute()
{
	if (CanExecute())
	{
		StartCooldown();
		
		// Handle resource costs
		if (UResourceManager* ResMgr = GetResourceManager())
		{
			// ADD WP for Hacker abilities (corruption system)
			if (WPCost > 0.0f)
			{
				ResMgr->AddWillPower(WPCost); // Changed from ConsumeWillPower to AddWillPower
			}

			// Consume Heat for Forge abilities
			if (HeatCost > 0.0f)
			{
				ResMgr->ConsumeHeat(HeatCost);
			}

			// For stamina consumption, we'd add:
			// if (StaminaCost > 0.0f) ResMgr->ConsumeStamina(StaminaCost);
			
			// Legacy heat generation (for abilities that don't use HeatCost)
			if (HeatCost <= 0.0f)
			{
				float HeatGenerated = CalculateHeatGenerated();
				ResMgr->AddHeat(HeatGenerated);
			}
		}
	}
}

float UAbilityComponent::GetCooldownPercentage() const
{
	return Cooldown > 0.0f ? CurrentCooldown / Cooldown : 0.0f;
}

void UAbilityComponent::StartCooldown()
{
	bIsOnCooldown = true;
	CurrentCooldown = GetCooldownWithReduction();
}

void UAbilityComponent::ResetCooldown()
{
	bIsOnCooldown = false;
	CurrentCooldown = 0.0f;
}

UResourceManager* UAbilityComponent::GetResourceManager() const
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UResourceManager>();
		}
	}
	return nullptr;
}

float UAbilityComponent::CalculateHeatGenerated() const
{
	return Cost * HeatGenerationMultiplier;
}

float UAbilityComponent::GetDamageMultiplier() const
{
	if (ThresholdManager)
	{
		const FSurvivorBuff& Buff = ThresholdManager->GetCurrentBuff();
		return Buff.DamageMultiplier;
	}
	return 1.0f;
}

float UAbilityComponent::GetCooldownWithReduction() const
{
	float FinalCooldown = Cooldown;
	
	if (ThresholdManager)
	{
		const FSurvivorBuff& Buff = ThresholdManager->GetCurrentBuff();
		FinalCooldown *= (1.0f - Buff.CooldownReduction);
	}
	
	return FinalCooldown;
}