#include "Components/Abilities/AbilityComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Config/GameplayConfig.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Movement/WallRunComponent.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // Optimize: only tick when needed
	
	Cooldown = GameplayConfig::Abilities::Defaults::COOLDOWN;
	WPCost = 0.0f;
	Range = GameplayConfig::Abilities::Defaults::RANGE;
	CurrentCooldown = 0.0f;
	bIsOnCooldown = false;
	bIsInUltimateMode = false;
	bIsBasicAbility = false; // Most abilities are not basic
	bIsDisabled = false; // Start enabled
	CurrentState = EAbilityState::Ready;
	bTickOnlyWhenActive = true;
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
	// DO NOT reset bIsDisabled - it should persist for the game session
	// bIsDisabled = false; // REMOVED - disabled abilities should stay disabled
	
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
			// Disable tick when not needed
			if (bTickOnlyWhenActive)
			{
				SetTickEnabled(false);
			}
		}
	}
}

bool UAbilityComponent::CanExecute() const
{
	// FIRST AND MOST IMPORTANT CHECK - permanently disabled abilities cannot execute
	if (bIsDisabled)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability %s: CanExecute() = FALSE - PERMANENTLY DISABLED after ultimate sacrifice!"), *GetName());
		return false;
	}
	
	// ULTIMATE MODE BYPASS - if in ultimate mode, skip most checks
	if (bIsInUltimateMode && !bIsBasicAbility)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s: Ultimate mode active - bypassing normal checks"), *GetName());
		
		// Only check if owner is alive
		if (AActor* Owner = GetOwner())
		{
			if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Owner))
			{
				return !PlayerOwner->IsDead();
			}
		}
		return true; // Allow ultimate execution
	}
	
	// Check ability state
	if (CurrentState == EAbilityState::Disabled || CurrentState == EAbilityState::Executing)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Ability %s: CanExecute() = FALSE - State is %d"), *GetName(), (int32)CurrentState);
		return false;
	}
	
	// Check if owner is dead
	if (AActor* Owner = GetOwner())
	{
		if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Owner))
		{
			if (PlayerOwner->IsDead())
			{
				return false;
			}
			
			// Check wall run restrictions - only allow certain abilities during wall running
			if (UWallRunComponent* WallRunComp = PlayerOwner->GetWallRunComponent())
			{
				if (!WallRunComp->CanUseAbilityDuringWallRun(this))
				{
					UE_LOG(LogTemp, Verbose, TEXT("Ability %s: CanExecute() = FALSE - Blocked during wall run"), *GetName());
					return false;
				}
			}
		}
	}
	else
	{
		return false;
	}
	
	if (bIsOnCooldown)
	{
		return false;
	}

	// Use new resource validation
	return ValidateResources();
}

bool UAbilityComponent::ValidateResources() const
{
	// If in ultimate mode, resources are not required
	if (bIsInUltimateMode)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Ability %s: Resource validation bypassed - in ultimate mode"), *GetName());
		return true;
	}
	
	// Get resource interface from owner
	if (IResourceConsumer* ResourceInterface = GetOwnerResourceInterface())
	{
		return ResourceInterface->Execute_HasResources(GetOwner(), 0.0f, WPCost);
	}
	
	// Fallback to old system if interface not implemented
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		float CurrentWP = ResMgr->GetCurrentWillPower();
		
		// If WP > 0, ALWAYS allow ability use (to trigger critical state)
		if (CurrentWP > 0.0f)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Ability %s: WP > 0 (%.1f) - allowing use regardless of cost (%.1f)"), 
				*GetName(), CurrentWP, WPCost);
			return true;
		}
		
		// If WP = 0, only block if not in ultimate mode
		if (CurrentWP <= 0.0f)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Ability %s: WP at 0, blocking unless in ultimate mode"), *GetName());
			return false; // Will be bypassed if in ultimate mode
		}
	}

	return true;
}

void UAbilityComponent::Execute()
{
	// Double-check disabled state
	if (bIsDisabled)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability %s: Execute called on DISABLED ability! This should not happen!"), *GetName());
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Ability %s: Execute() called, bIsInUltimateMode=%s, bIsDisabled=%s"), 
		*GetName(), bIsInUltimateMode ? TEXT("TRUE") : TEXT("FALSE"), bIsDisabled ? TEXT("TRUE") : TEXT("FALSE"));
	
	if (CanExecute())
	{
		SetAbilityState(EAbilityState::Executing);
		
		// Check if we're in ultimate mode
		if (bIsInUltimateMode && !bIsBasicAbility)
		{
			// Execute ultimate version - no resource cost, no cooldown
			UE_LOG(LogTemp, Warning, TEXT("Ability %s: Executing ULTIMATE version!"), *GetName());
			ExecuteUltimate();
			
			// Notify threshold manager that an ultimate was used
			if (UWorld* World = GetWorld())
			{
				if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
				{
					ThresholdMgr->OnAbilityExecuted(this);
				}
			}
			
			// Ultimate abilities go back to Ready state, not Cooldown
			SetAbilityState(EAbilityState::Ready);
		}
		else
		{
			// Normal ability execution
			StartCooldown();
			
			// Use new resource consumption
			ConsumeAbilityResources();
			
			SetAbilityState(EAbilityState::Cooldown);
		}
	}
}

void UAbilityComponent::ExecuteUltimate()
{
	// Default implementation - abilities will override this
	UE_LOG(LogTemp, Warning, TEXT("Ability %s: ExecuteUltimate (base implementation, IsInUltimateMode=%s)"), 
		*GetName(), bIsInUltimateMode ? TEXT("TRUE") : TEXT("FALSE"));
	
	// Ultimate abilities have no resource costs and no cooldowns
	// Do NOT consume resources or start cooldown
	
	// Ensure the ability is marked as being in ultimate mode
	if (!bIsInUltimateMode)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability %s: ExecuteUltimate called but bIsInUltimateMode is FALSE! Setting to TRUE"), *GetName());
		bIsInUltimateMode = true;
	}
}

void UAbilityComponent::SetUltimateMode(bool bEnabled)
{
	bIsInUltimateMode = bEnabled;
	
	if (bEnabled && !bIsBasicAbility)
	{
		// Clear cooldown when entering ultimate mode
		bIsOnCooldown = false;
		CurrentCooldown = 0.0f;
		
		// Reset ability state to ready
		if (CurrentState == EAbilityState::Cooldown)
		{
			SetAbilityState(EAbilityState::Ready);
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Ability %s: Ultimate mode enabled - cooldown cleared"), *GetName());
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
		// Enable tick for cooldown countdown
		if (bTickOnlyWhenActive)
		{
			SetTickEnabled(true);
		}
	}
	else
	{
		// No cooldown needed
		bIsOnCooldown = false;
		CurrentCooldown = 0.0f;
		SetAbilityState(EAbilityState::Ready);
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

void UAbilityComponent::SetTickEnabled(bool bEnabled)
{
	SetComponentTickEnabled(bEnabled);
}

void UAbilityComponent::SetAbilityState(EAbilityState NewState)
{
	CurrentState = NewState;
}

IResourceConsumer* UAbilityComponent::GetOwnerResourceInterface() const
{
	if (AActor* Owner = GetOwner())
	{
		return Cast<IResourceConsumer>(Owner);
	}
	return nullptr;
}

bool UAbilityComponent::ConsumeAbilityResources()
{
	// BLACKHOLE ENERGY SYSTEM:
	// Abilities consume WP (Will Power) as an energy/mana resource:
	// - Using abilities consumes WP (reduces energy)
	// - Killing enemies restores WP
	// - At 0% WP: Death from energy depletion
	// - Basic abilities (slash, dash, jump) cost 0 WP
	
	// Try interface first
	if (IResourceConsumer* ResourceInterface = GetOwnerResourceInterface())
	{
		// Consume WP as energy cost
		return ResourceInterface->Execute_ConsumeResources(GetOwner(), 0.0f, WPCost);
	}
	
	// Fallback to old system
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		// Consume WP as energy cost
		if (WPCost > 0.0f)
		{
			if (!ResMgr->ConsumeWillPower(WPCost))
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: Not enough WP to use ability (need %.1f)"), *GetName(), WPCost);
				return false;
			}
			UE_LOG(LogTemp, VeryVerbose, TEXT("%s consumed %.1f WP"), *GetName(), WPCost);
		}

		return true;
	}
	return false;
}