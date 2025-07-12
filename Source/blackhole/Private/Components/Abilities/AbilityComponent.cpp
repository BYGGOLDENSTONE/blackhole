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
		// No additional checks needed

		// WP validation: At 100% WP, abilities work differently
		if (WPCost > 0.0f)
		{
			float CurrentWP = ResMgr->GetCurrentWillPower();
			float MaxWP = ResMgr->GetMaxWillPower();
			
			// At 100% WP, non-basic abilities are allowed (they'll become ultimates)
			if (CurrentWP >= MaxWP && !bIsBasicAbility)
			{
				UE_LOG(LogTemp, Warning, TEXT("Ability %s: Allowed at 100%% WP - will trigger ultimate mode"), *GetName());
				return true;
			}
			// Basic abilities always work
			else if (bIsBasicAbility)
			{
				return true;
			}
			// Below 100% WP, check normal resource availability
			else if (CurrentWP < MaxWP)
			{
				return true; // Allow all abilities below 100% WP
			}
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
	
	// Check if we're at 100% WP and need ultimate mode
	bool bShouldBeUltimate = false;
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		float CurrentWP = ResMgr->GetCurrentWillPower();
		float MaxWP = ResMgr->GetMaxWillPower();
		
		// If at 100% WP and not a basic ability, this MUST be an ultimate
		if (CurrentWP >= MaxWP && !bIsBasicAbility)
		{
			bShouldBeUltimate = true;
			
			if (UWorld* World = GetWorld())
			{
				if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
				{
					// Ensure combat is started
					if (!ThresholdMgr->IsInCombat())
					{
						UE_LOG(LogTemp, Warning, TEXT("Ability %s: Starting combat for ultimate mode"), *GetName());
						ThresholdMgr->StartCombat();
					}
					
					// Force activate ultimate mode if not already active
					if (!ThresholdMgr->IsUltimateModeActive())
					{
						UE_LOG(LogTemp, Warning, TEXT("Ability %s: Force activating ultimate mode"), *GetName());
						ThresholdMgr->ActivateUltimateMode();
					}
					
					// Now set this ability to ultimate mode
					bIsInUltimateMode = true;
					UE_LOG(LogTemp, Error, TEXT("Ability %s: Set to ultimate mode due to 100%% WP"), *GetName());
				}
			}
		}
		
		if (CurrentWP >= MaxWP * 0.9f) // Log when close to or at 100% WP
		{
			UE_LOG(LogTemp, Warning, TEXT("Ability %s: Execute called with WP=%.1f/%.1f (%.1f%%), UltimateMode=%s, ShouldBeUltimate=%s"), 
				*GetName(), CurrentWP, MaxWP, (CurrentWP/MaxWP)*100.0f, 
				bIsInUltimateMode ? TEXT("TRUE") : TEXT("FALSE"),
				bShouldBeUltimate ? TEXT("TRUE") : TEXT("FALSE"));
		}
	}
	
	if (CanExecute())
	{
		SetAbilityState(EAbilityState::Executing);
		
		// If this should be ultimate, execute as ultimate (no resource cost)
		if (bShouldBeUltimate || bIsInUltimateMode)
		{
			UE_LOG(LogTemp, Error, TEXT("!!! ULTIMATE ABILITY EXECUTING !!!"));
			UE_LOG(LogTemp, Error, TEXT("Ability %s: Executing ULTIMATE version! (bShouldBeUltimate=%s, bIsInUltimateMode=%s)"), 
				*GetName(), bShouldBeUltimate ? TEXT("TRUE") : TEXT("FALSE"), 
				bIsInUltimateMode ? TEXT("TRUE") : TEXT("FALSE"));
			
			// First execute the ultimate ability
			ExecuteUltimate();
			
			// Set state back to ready since ultimates don't have cooldowns
			SetAbilityState(EAbilityState::Ready);
			
			// ONLY notify ThresholdManager after successfully executing the ultimate
			// This ensures the ultimate actually fired before we disable it
			if (UWorld* World = GetWorld())
			{
				if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
				{
					UE_LOG(LogTemp, Warning, TEXT("Ability %s: Notifying ThresholdManager of ultimate execution"), *GetName());
					ThresholdMgr->OnAbilityExecuted(this);
				}
			}
			
			return;
		}
		
		// Normal ability execution
		StartCooldown();
		
		// Use new resource consumption
		ConsumeAbilityResources();
		
		SetAbilityState(EAbilityState::Cooldown);
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
	// BLACKHOLE CORRUPTION SYSTEM:
	// Unlike traditional resource systems, abilities in Blackhole ADD corruption (WP)
	// rather than consuming it. This creates a risk/reward dynamic where:
	// - Using abilities corrupts the player (increases WP)
	// - At 50% WP: Player gets combat buffs
	// - At 100% WP: Abilities transform into ultimates
	// - Using an ultimate permanently disables that ability
	// - Death occurs after losing 3 abilities or reaching 100% WP for the 4th time
	
	// Try interface first
	if (IResourceConsumer* ResourceInterface = GetOwnerResourceInterface())
	{
		// Note: Despite the function name, this ADDS corruption
		return ResourceInterface->Execute_ConsumeResources(GetOwner(), 0.0f, WPCost);
	}
	
	// Fallback to old system
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		// ADD WP corruption (NOT consume) - this is intentional!
		if (WPCost > 0.0f)
		{
			ResMgr->AddWillPower(WPCost);
			UE_LOG(LogTemp, VeryVerbose, TEXT("%s corrupted player with +%.1f WP"), *GetName(), WPCost);
		}

		// Always return true - corruption cannot fail
		return true;
	}
	return false;
}