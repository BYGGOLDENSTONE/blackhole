#include "Components/Abilities/AbilityComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/Attributes/StaminaComponent.h"

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
		UE_LOG(LogTemp, Warning, TEXT("Ability %s: Cannot execute - on cooldown"), *GetName());
		return false;
	}

	// Check resource availability
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		// Check stamina availability
		if (StaminaCost > 0.0f)
		{
			// Check if owner has a stamina component
			if (AActor* Owner = GetOwner())
			{
				if (UStaminaComponent* StaminaComp = Owner->FindComponentByClass<UStaminaComponent>())
				{
					if (!StaminaComp->HasEnoughStamina(StaminaCost))
					{
						UE_LOG(LogTemp, Warning, TEXT("Ability %s: Cannot execute - not enough stamina (need %.0f)"), *GetName(), StaminaCost);
						return false;
					}
				}
			}
		}

		// WP system is inverted: Hacker abilities ADD WP (corruption)
		// No need to check WP availability since it's added, not consumed
		// However, we might want to prevent going over 100% in the future
		
		// For Forge abilities, check if adding heat would cause overheat
		if (HeatCost > 0.0f && ResMgr->IsOverheated())
		{
			UE_LOG(LogTemp, Warning, TEXT("Ability %s: Cannot execute - system overheated"), *GetName());
			return false; // Can't use abilities while overheated
		}
	}

	return true;
}

void UAbilityComponent::Execute()
{
	UE_LOG(LogTemp, Warning, TEXT("Ability %s: Execute called"), *GetName());
	
	if (CanExecute())
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s: Executing (Stamina: %.0f, WP: %.0f, Heat: %.0f)"), 
			*GetName(), StaminaCost, WPCost, HeatCost);
		
		StartCooldown();
		
		// Handle resource costs
		if (UResourceManager* ResMgr = GetResourceManager())
		{
			// ADD WP for Hacker abilities (corruption system)
			if (WPCost > 0.0f)
			{
				ResMgr->AddWillPower(WPCost); // Changed from ConsumeWillPower to AddWillPower
			}

			// ADD Heat for Forge abilities (heat is a penalty resource)
			if (HeatCost > 0.0f)
			{
				ResMgr->AddHeat(HeatCost);
			}

			// Consume stamina
			if (StaminaCost > 0.0f)
			{
				if (AActor* Owner = GetOwner())
				{
					if (UStaminaComponent* StaminaComp = Owner->FindComponentByClass<UStaminaComponent>())
					{
						StaminaComp->UseStamina(StaminaCost);
					}
				}
			}
			
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
	float CooldownDuration = GetCooldownWithReduction();
	if (CooldownDuration > 0.0f)
	{
		bIsOnCooldown = true;
		CurrentCooldown = CooldownDuration;
	}
	else
	{
		// No cooldown needed
		bIsOnCooldown = false;
		CurrentCooldown = 0.0f;
	}
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