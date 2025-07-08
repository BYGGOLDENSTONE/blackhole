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
	bIsInUltimateMode = false;
	bIsBasicAbility = false; // Most abilities are not basic
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
		// Don't log cooldown checks - this gets called every tick
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
						// Don't log resource checks - this gets called every tick
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
			// Don't log overheat checks - this gets called every tick
			return false; // Can't use abilities while overheated
		}
	}

	return true;
}

void UAbilityComponent::Execute()
{
	// Only log ultimate executions
	if (bIsInUltimateMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s: ULTIMATE Execute!"), *GetName());
	}
	
	if (CanExecute())
	{
		// If in ultimate mode, execute ultimate version instead
		if (bIsInUltimateMode)
		{
			ExecuteUltimate();
			
			// Notify ThresholdManager that this ability was used in ultimate mode
			if (UWorld* World = GetWorld())
			{
				if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
				{
					UE_LOG(LogTemp, Warning, TEXT("Ability %s: Notifying ThresholdManager of ultimate execution"), *GetName());
					ThresholdMgr->OnAbilityExecuted(this);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Ability %s: Could not find ThresholdManager!"), *GetName());
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Ability %s: No World context!"), *GetName());
			}
			return;
		}
		
		// Removed normal execution logging to reduce spam
		
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

void UAbilityComponent::ExecuteUltimate()
{
	// Default implementation - abilities will override this
	UE_LOG(LogTemp, Warning, TEXT("Ability %s: ExecuteUltimate (base implementation)"), *GetName());
	
	// No resource costs for ultimate abilities
	StartCooldown();
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