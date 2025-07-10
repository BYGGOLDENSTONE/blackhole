#include "Components/Abilities/AbilityComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/Attributes/StaminaComponent.h"
#include "Config/GameplayConfig.h"
#include "Player/BlackholePlayerCharacter.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	Cooldown = GameplayConfig::Abilities::Defaults::COOLDOWN;
	Cost = 0.0f; // Legacy field
	StaminaCost = 0.0f;
	WPCost = 0.0f;
	Range = GameplayConfig::Abilities::Defaults::RANGE;
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

void UAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Reset cooldown state to prevent persistence between PIE sessions
	CurrentCooldown = 0.0f;
	bIsOnCooldown = false;
	bIsInUltimateMode = false;
	
	// Clear cached references
	ResourceManager = nullptr;
	ThresholdManager = nullptr;
	
	Super::EndPlay(EndPlayReason);
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
	// Check if owner is dead
	if (AActor* Owner = GetOwner())
	{
		if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Owner))
		{
			if (PlayerOwner->IsDead())
			{
				return false;
			}
		}
	}
	else
	{
		// No owner - ability cannot execute
		return false;
	}
	
	// Check if ability is disabled (component tick disabled)
	if (!IsComponentTickEnabled())
	{
		return false; // Ability has been disabled by ThresholdManager
	}
	
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
			else
			{
				// No owner to check stamina on
				return false;
			}
		}

		// WP validation: Only block if we're already at 100% WP and in ultimate mode
		if (WPCost > 0.0f)
		{
			float CurrentWP = ResMgr->GetCurrentWillPower();
			float MaxWP = ResMgr->GetMaxWillPower();
			
			// Only block if we're already at 100% WP - allow abilities that reach 100%
			if (CurrentWP >= MaxWP)
			{
				// Basic abilities (like slash) should always work regardless of WP level
				if (bIsBasicAbility)
				{
					UE_LOG(LogTemp, Log, TEXT("Ability %s: Allowed - basic ability at 100%% WP"), *GetName());
				}
				// At 100% WP, only ultimate abilities or basic abilities should work
				else if (!bIsInUltimateMode)
				{
					if (CurrentWP >= MaxWP * 0.9f) // Debug log
					{
						UE_LOG(LogTemp, Warning, TEXT("Ability %s: Blocked - at 100%% WP but not in ultimate mode"), *GetName());
					}
					return false; // Regular abilities blocked at 100% WP
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Ability %s: Allowed - at 100%% WP and in ultimate mode"), *GetName());
				}
			}
			// Allow all abilities below 100% WP, even if they would reach 100%
		}
	}

	return true;
}

void UAbilityComponent::Execute()
{
	// Debug logging for WP issues
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		float CurrentWP = ResMgr->GetCurrentWillPower();
		float MaxWP = ResMgr->GetMaxWillPower();
		if (CurrentWP >= MaxWP * 0.9f) // Log when close to or at 100% WP
		{
			UE_LOG(LogTemp, Warning, TEXT("Ability %s: Execute called with WP=%.1f/%.1f (%.1f%%), UltimateMode=%s, CanExecute=%s"), 
				*GetName(), CurrentWP, MaxWP, (CurrentWP/MaxWP)*100.0f, 
				bIsInUltimateMode ? TEXT("TRUE") : TEXT("FALSE"),
				CanExecute() ? TEXT("TRUE") : TEXT("FALSE"));
		}
	}
	
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
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Ability %s: Cannot consume stamina - no owner"), *GetName());
				}
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


float UAbilityComponent::GetDamageMultiplier() const
{
	// Use default value if ThresholdManager is not available
	// This prevents crashes when the subsystem isn't initialized yet
	return 1.0f;
}

float UAbilityComponent::GetCooldownWithReduction() const
{
	// Use base cooldown without reduction for now
	// This prevents crashes when the subsystem isn't initialized yet
	return Cooldown;
}