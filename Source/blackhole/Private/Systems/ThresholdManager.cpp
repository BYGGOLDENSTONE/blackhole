#include "Systems/ThresholdManager.h"
#include "Systems/ResourceManager.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Components/Abilities/UtilityAbility.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Config/GameplayConfig.h"
#include "TimerManager.h"

void UThresholdManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Get player character
	PlayerCharacter = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	// Get resource manager and bind to threshold changes
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		ResourceManager = GameInstance->GetSubsystem<UResourceManager>();
		if (ResourceManager)
		{
			ResourceManager->OnWillPowerThresholdReached.AddDynamic(this, &UThresholdManager::OnWPThresholdChanged);
			ResourceManager->OnWPMaxReached.AddDynamic(this, &UThresholdManager::OnWPMaxReachedHandler);
		}
	}
}

void UThresholdManager::Deinitialize()
{
	// Unbind from resource manager
	if (IsValid(ResourceManager))
	{
		ResourceManager->OnWillPowerThresholdReached.RemoveDynamic(this, &UThresholdManager::OnWPThresholdChanged);
		ResourceManager->OnWPMaxReached.RemoveDynamic(this, &UThresholdManager::OnWPMaxReachedHandler);
	}
	
	// Clear critical timer
	StopCriticalTimer();
	
	// Clear all delegate bindings to prevent crashes
	OnCombatStarted.Clear();
	OnCombatEnded.Clear();
	OnAbilityDisabled.Clear();
	OnSurvivorBuff.Clear();
	OnUltimateModeActivated.Clear();
	OnPlayerDeath.Clear();
	OnCriticalTimer.Clear();
	OnCriticalTimerExpired.Clear();
	
	// Clear ability arrays
	AllPlayerAbilities.Empty();
	DisabledAbilities.Empty();
	
	Super::Deinitialize();
}

void UThresholdManager::StartCombat()
{
	if (bIsInCombat)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: StartCombat called but already in combat"));
		return;
	}
	
	bIsInCombat = true;
	
	// Reset threshold state for new combat
	ThresholdState.Reset();
	// DO NOT clear DisabledAbilities here - they should persist through combat
	// DisabledAbilities.Empty(); // REMOVED - disabled abilities should stay disabled
	CurrentBuff = FSurvivorBuff();
	
	// Try to get player character if we don't have it yet
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: StartCombat - Getting player character: %s"), 
			PlayerCharacter ? TEXT("Found") : TEXT("Not Found"));
	}
	
	// Cache player abilities
	CachePlayerAbilities();
	
	OnCombatStarted.Broadcast();
	
	UE_LOG(LogTemp, Log, TEXT("ThresholdManager: Combat started"));
}

void UThresholdManager::EndCombat()
{
	if (!bIsInCombat)
	{
		return;
	}
	
	// DEBUG: Log call stack to find what's ending combat (which clears critical timer)
	const FString CallStack = FFrame::GetScriptCallstack();
	UE_LOG(LogTemp, Error, TEXT("!!! ENDCOMBAT CALLED (WILL CLEAR CRITICAL TIMER) !!! Call stack:\n%s"), *CallStack);
	
	bIsInCombat = false;
	bUltimateModeActive = false;
	bIsDeactivatingUltimate = false;
	
	// Clean up invalid abilities before processing
	CleanupInvalidAbilities();
	
	// Clear all timers for this object
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	
	// DO NOT re-enable abilities that were disabled by ultimate sacrifice
	// Those should remain permanently disabled
	// Just clear the tracking arrays
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Combat ended, keeping %d abilities disabled from ultimate sacrifice"), DisabledAbilities.Num());
	
	// Reset ultimate mode for all abilities
	for (int32 i = AllPlayerAbilities.Num() - 1; i >= 0; i--)
	{
		UAbilityComponent* Ability = AllPlayerAbilities[i];
		if (IsValid(Ability))
		{
			Ability->SetUltimateMode(false);
		}
		else
		{
			AllPlayerAbilities.RemoveAt(i);
		}
	}
	
	// DO NOT clear DisabledAbilities - we need to track which abilities were sacrificed
	// DisabledAbilities.Empty(); // REMOVED - keep tracking sacrificed abilities
	AllPlayerAbilities.Empty(); // Clear the array to prevent stale references
	CurrentBuff = FSurvivorBuff();
	
	OnCombatEnded.Broadcast();
	
	UE_LOG(LogTemp, Log, TEXT("ThresholdManager: Combat ended, all abilities restored"));
}

bool UThresholdManager::IsAbilityDisabled(UAbilityComponent* Ability) const
{
	return DisabledAbilities.Contains(Ability);
}

void UThresholdManager::OnWPThresholdChanged(EResourceThreshold NewThreshold)
{
	if (!bIsInCombat)
	{
		return;
	}
	
	// Clean up invalid abilities before processing
	CleanupInvalidAbilities();
	
	// Handle new threshold system
	switch (NewThreshold)
	{
		case EResourceThreshold::Normal: // 0-50%
			// Remove buff if going back to normal
			if (CurrentBuff.bIsBuffed)
			{
				CurrentBuff.bIsBuffed = false;
				UpdateSurvivorBuffs();
			}
			break;
			
		case EResourceThreshold::Buffed: // 50-100%
			// Apply buff at 50% WP
			if (!CurrentBuff.bIsBuffed)
			{
				CurrentBuff.bIsBuffed = true;
				UpdateSurvivorBuffs();
				UE_LOG(LogTemp, Log, TEXT("ThresholdManager: 50%% WP reached - Abilities buffed!"));
			}
			break;
			
		case EResourceThreshold::Critical: // 100% - handled by OnWPMaxReachedHandler
			// This shouldn't be reached as we handle 100% separately
			break;
			
		default:
			break;
	}
}


void UThresholdManager::OnWPMaxReachedHandler(int32 TimesReached)
{
	UE_LOG(LogTemp, Error, TEXT("=== ThresholdManager::OnWPMaxReachedHandler START ==="));
	UE_LOG(LogTemp, Error, TEXT("WP reached 100%% - Count: %d, InCombat: %s"), TimesReached, bIsInCombat ? TEXT("YES") : TEXT("NO"));
	
	if (!bIsInCombat)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Not in combat, starting combat now"));
		StartCombat();
	}
	
	// Clean up invalid abilities before processing
	CleanupInvalidAbilities();
	
	// Ensure ResourceManager is valid
	if (!IsValid(ResourceManager))
	{
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: ResourceManager is null in OnWPMaxReachedHandler!"));
		return;
	}
	
	// Double-check current WP
	float CurrentWP = ResourceManager->GetCurrentWillPower();
	float MaxWP = ResourceManager->GetMaxWillPower();
	UE_LOG(LogTemp, Error, TEXT("ThresholdManager: Current WP = %.1f/%.1f"), CurrentWP, MaxWP);
	
	// Only apply ultimate mode for Hacker path
	if (ResourceManager->GetCurrentPath() != ECharacterPath::Hacker)
	{
		UE_LOG(LogTemp, Log, TEXT("ThresholdManager: WP reached 100%% but player is on Forge path - ignoring"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: WP reached 100%% (Count: %d) - PREPARING TO ACTIVATE ULTIMATE MODE!"), TimesReached);
	
	// Check death conditions - but NOT for immediate death, only for critical timer behavior
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Checking death conditions - DisabledAbilities: %d/%d, TimesReached: %d/%d"), 
		DisabledAbilities.Num(), GameplayConfig::Thresholds::MAX_DISABLED_ABILITIES,
		TimesReached, GameplayConfig::Thresholds::MAX_WP_REACHES);
		
	// ONLY trigger immediate death if player has lost 3 abilities AND this is another 100% reach
	// The 4th time reaching 100% should give player a chance via critical timer, not instant death
	if (DisabledAbilities.Num() >= GameplayConfig::Thresholds::MAX_DISABLED_ABILITIES)
	{
		// Player has lost 3 abilities - any additional 100% WP reach is death
		EndCombat();
		OnPlayerDeath.Broadcast();
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: Player death triggered - WP reached 100%% after losing %d abilities!"), DisabledAbilities.Num());
		return;
	}
	
	// DO NOT trigger immediate death for TimesReached >= 4
	// Instead, let critical timer handle it - player gets 5 seconds to use ultimate
	if (TimesReached >= GameplayConfig::Thresholds::MAX_WP_REACHES)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: This is the %d time reaching 100%% WP - FINAL CHANCE via critical timer!"), TimesReached);
		// Don't return - continue to critical timer
	}
	
	// Start critical timer - player has 5 seconds to use ultimate or die
	StartCriticalTimer();
	
	// Double-check WP after starting timer
	CurrentWP = ResourceManager->GetCurrentWillPower();
	UE_LOG(LogTemp, Error, TEXT("=== ThresholdManager::OnWPMaxReachedHandler END - Critical Timer Started - WP = %.1f/%.1f ==="), CurrentWP, MaxWP);
}

void UThresholdManager::ActivateUltimateMode()
{
	if (bUltimateModeActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Ultimate mode already active"));
		return;
	}
	
	// Check current WP before activation
	if (IsValid(ResourceManager))
	{
		float CurrentWP = ResourceManager->GetCurrentWillPower();
		float MaxWP = ResourceManager->GetMaxWillPower();
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Activating ultimate mode with WP=%.1f/%.1f"), CurrentWP, MaxWP);
	}
	
	bUltimateModeActive = true;
	UltimateModeActivationTime = GetWorld()->GetTimeSeconds();
	
	// Clean up invalid abilities before processing
	CleanupInvalidAbilities();
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Activating ultimate mode, checking %d abilities"), AllPlayerAbilities.Num());
	
	// Set all non-basic abilities to ultimate mode
	int32 UltimateCount = 0;
	for (int32 i = AllPlayerAbilities.Num() - 1; i >= 0; i--) // Iterate backwards for safe removal
	{
		UAbilityComponent* Ability = AllPlayerAbilities[i];
		if (!IsValid(Ability))
		{
			AllPlayerAbilities.RemoveAt(i);
			continue;
		}
		
		if (!DisabledAbilities.Contains(Ability))
		{
			// Skip basic abilities - they don't have ultimate versions
			if (!Ability->IsBasicAbility())
			{
				Ability->SetUltimateMode(true);
				UltimateCount++;
				UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Set %s to ultimate mode (IsBasic=%s)"), 
					*Ability->GetName(), Ability->IsBasicAbility() ? TEXT("YES") : TEXT("NO"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Skipped %s - marked as basic ability"), 
					*Ability->GetName());
			}
		}
	}
	
	OnUltimateModeActivated.Broadcast(true);
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: ULTIMATE MODE ACTIVATED - %d abilities enhanced!"), UltimateCount);
}

void UThresholdManager::CleanupInvalidAbilities()
{
	// Remove invalid abilities from AllPlayerAbilities
	for (int32 i = AllPlayerAbilities.Num() - 1; i >= 0; i--)
	{
		if (!IsValid(AllPlayerAbilities[i]))
		{
			UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Removing invalid ability at index %d"), i);
			AllPlayerAbilities.RemoveAt(i);
		}
	}
	
	// Remove invalid abilities from DisabledAbilities
	for (int32 i = DisabledAbilities.Num() - 1; i >= 0; i--)
	{
		if (!IsValid(DisabledAbilities[i]))
		{
			UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Removing invalid disabled ability at index %d"), i);
			DisabledAbilities.RemoveAt(i);
		}
	}
}

void UThresholdManager::DeactivateUltimateMode(UAbilityComponent* UsedAbility)
{
	if (!bUltimateModeActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: DeactivateUltimateMode called but ultimate mode is not active"));
		return;
	}
	
	// DEBUG: Log call stack to find what's deactivating ultimate mode (which resets WP)
	const FString CallStack = FFrame::GetScriptCallstack();
	UE_LOG(LogTemp, Error, TEXT("!!! DEACTIVATE ULTIMATE MODE CALLED (WILL RESET WP) !!! Ability: %s\nCall stack:\n%s"), 
		UsedAbility ? *UsedAbility->GetName() : TEXT("NULL"), *CallStack);
	
	// Prevent recursive calls
	if (bIsDeactivatingUltimate)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Already deactivating ultimate mode, preventing recursion"));
		return;
	}
	
	// Basic abilities don't trigger deactivation
	if (IsValid(UsedAbility) && UsedAbility->IsBasicAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Basic ability used during ultimate mode - no deactivation"));
		return;
	}
	
	UE_LOG(LogTemp, Error, TEXT("ThresholdManager: DEACTIVATING ULTIMATE MODE - Used ability: %s"), 
		UsedAbility ? *UsedAbility->GetName() : TEXT("NULL"));
	
	bIsDeactivatingUltimate = true;
	bUltimateModeActive = false;
	
	// Clean up invalid abilities before processing
	CleanupInvalidAbilities();
	
	// Deactivate ultimate mode for all abilities
	for (int32 i = AllPlayerAbilities.Num() - 1; i >= 0; i--)
	{
		UAbilityComponent* Ability = AllPlayerAbilities[i];
		if (IsValid(Ability))
		{
			Ability->SetUltimateMode(false);
		}
		else
		{
			AllPlayerAbilities.RemoveAt(i);
		}
	}
	
	// Disable the ability that was used (if it's not basic)
	if (IsValid(UsedAbility) && !DisabledAbilities.Contains(UsedAbility) && !UsedAbility->IsBasicAbility())
	{
		// Properly disable the ability
		UsedAbility->SetDisabled(true);
		UsedAbility->SetComponentTickEnabled(false);
		UsedAbility->SetAbilityState(EAbilityState::Disabled);
		DisabledAbilities.Add(UsedAbility);
		
		// Verify the ability is actually disabled
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: Disabling %s - IsDisabled=%s"), 
			*UsedAbility->GetName(), 
			UsedAbility->IsDisabled() ? TEXT("TRUE") : TEXT("FALSE"));
		
		// Update buffs for lost abilities
		UpdateSurvivorBuffs();
		
		// Broadcast event
		OnAbilityDisabled.Broadcast(UsedAbility, DisabledAbilities.Num());
		
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: PERMANENTLY DISABLED ability %s after ultimate use (Total disabled: %d)"), 
			*UsedAbility->GetName(), DisabledAbilities.Num());
	}
	
	// Reset WP to 0 - ONLY when ultimate ability is actually used
	if (IsValid(ResourceManager))
	{
		// Clear critical state first
		ResourceManager->SetCriticalState(false);
		
		// Authorize the reset - this is the ONLY legitimate place to reset WP
		ResourceManager->AuthorizeWPReset();
		ResourceManager->ResetWPAfterMax();
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: Authorized WP reset after ultimate ability use"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: ResourceManager is null in DeactivateUltimateMode!"));
	}
	
	OnUltimateModeActivated.Broadcast(false);
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Ultimate mode deactivated"));
	
	// Clear the deactivation flag
	bIsDeactivatingUltimate = false;
}

void UThresholdManager::OnAbilityExecuted(UAbilityComponent* Ability)
{
	// Safety check
	if (!Ability)
	{
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: OnAbilityExecuted called with NULL ability!"));
		return;
	}
	
	// CRITICAL FIX: Only process PLAYER abilities, ignore enemy abilities
	if (!PlayerCharacter || Ability->GetOwner() != PlayerCharacter)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("ThresholdManager: Ignoring enemy ability %s"), *Ability->GetName());
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: OnAbilityExecuted called for %s (UltimateMode: %s, IsBasic: %s, TimeSince: %.2f)"), 
		*Ability->GetName(),
		bUltimateModeActive ? TEXT("YES") : TEXT("NO"),
		Ability->IsBasicAbility() ? TEXT("YES") : TEXT("NO"),
		GetWorld() ? GetWorld()->GetTimeSeconds() - UltimateModeActivationTime : 0.0f);
	
	// Check if critical timer is active - any non-basic ability use should be treated as ultimate
	if (bCriticalTimerActive && !Ability->IsBasicAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Critical timer active, non-basic ability used - treating as ultimate ability"));
		StopCriticalTimer();
		
		// If ultimate mode wasn't active yet, activate it first
		if (!bUltimateModeActive)
		{
			ActivateUltimateMode();
		}
		
		// Treat this ability use as an ultimate and deactivate
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Ultimate ability %s executed during critical timer - deactivating ultimate mode"), *Ability->GetName());
		DeactivateUltimateMode(Ability);
		return; // Don't continue with normal processing
	}
	
	// Only process if we're actually in ultimate mode
	if (!bUltimateModeActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Not in ultimate mode, ignoring ability execution"));
		return;
	}
	
	// Basic abilities don't trigger deactivation
	if (Ability->IsBasicAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Basic ability used during ultimate mode - no deactivation"));
		return;
	}
	
	// We should ONLY deactivate ultimate mode if an ultimate ability was actually used
	if (Ability->IsInUltimateMode())
	{
		// This is an ultimate ability - deactivate ultimate mode after it's used
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Ultimate ability %s executed - deactivating ultimate mode"), *Ability->GetName());
		DeactivateUltimateMode(Ability);
	}
	else
	{
		// Non-ultimate ability during ultimate mode - should NOT deactivate
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Non-ultimate ability %s used during ultimate mode - IGNORING (not deactivating)"), *Ability->GetName());
		// Do NOT deactivate ultimate mode for non-ultimate abilities
		// Players should be able to use basic abilities without losing ultimate mode
	}
}

void UThresholdManager::UpdateSurvivorBuffs()
{
	int32 DisabledCount = DisabledAbilities.Num();
	
	// Base buffs from 50% WP threshold
	if (CurrentBuff.bIsBuffed)
	{
		CurrentBuff.DamageMultiplier = GameplayConfig::Thresholds::BUFFED_DAMAGE_MULT; // Damage increase at 50% WP
		CurrentBuff.CooldownReduction = GameplayConfig::Thresholds::BUFFED_COOLDOWN_REDUCTION; // Cooldown reduction at 50% WP
		CurrentBuff.AttackSpeed = GameplayConfig::Thresholds::BUFFED_ATTACK_SPEED; // Faster attack speed at 50% WP
	}
	else
	{
		CurrentBuff.DamageMultiplier = 1.0f;
		CurrentBuff.CooldownReduction = 0.0f;
		CurrentBuff.AttackSpeed = 1.0f;
	}
	
	// Additional buffs for each lost ability
	if (DisabledCount > 0)
	{
		CurrentBuff.DamageMultiplier += (GameplayConfig::Thresholds::DAMAGE_PER_LOST_ABILITY * DisabledCount); // Per lost ability
		CurrentBuff.CooldownReduction += (GameplayConfig::Thresholds::CDR_PER_LOST_ABILITY * DisabledCount); // Per lost ability
		CurrentBuff.AttackSpeed += (GameplayConfig::Thresholds::SPEED_PER_LOST_ABILITY * DisabledCount); // Per lost ability
	}
	
	// Broadcast buff update
	OnSurvivorBuff.Broadcast(CurrentBuff);
	
	UE_LOG(LogTemp, Log, TEXT("ThresholdManager: Buffs - Damage: %.0f%%, Cooldown: -%.0f%%, Attack Speed: %.0f%%"), 
		CurrentBuff.DamageMultiplier * 100.0f, 
		CurrentBuff.CooldownReduction * 100.0f,
		CurrentBuff.AttackSpeed * 100.0f);
}

void UThresholdManager::CachePlayerAbilities()
{
	AllPlayerAbilities.Empty();
	
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: No player character to cache abilities from"));
		return;
	}
	
	// Get all ability components on player
	TArray<UActorComponent*> Components = PlayerCharacter->GetComponents().Array();
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Found %d total components on player"), Components.Num());
	
	for (UActorComponent* Component : Components)
	{
		if (UAbilityComponent* Ability = Cast<UAbilityComponent>(Component))
		{
			// Include ALL abilities - basic abilities will be handled by checking bIsBasicAbility
			AllPlayerAbilities.Add(Ability);
			UE_LOG(LogTemp, VeryVerbose, TEXT("ThresholdManager: Added ability %s"), *Ability->GetName());
		}
	}
	
	// If we didn't find any abilities, try again with a more thorough search
	if (AllPlayerAbilities.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: No abilities found with GetComponents, trying FindComponents"));
		PlayerCharacter->GetComponents<UAbilityComponent>(AllPlayerAbilities);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Cached %d player abilities total"), AllPlayerAbilities.Num());
}

UAbilityComponent* UThresholdManager::GetRandomEnabledAbility() const
{
	TArray<UAbilityComponent*> EnabledAbilities;
	
	// Find all abilities that aren't disabled yet
	for (UAbilityComponent* Ability : AllPlayerAbilities)
	{
		if (IsValid(Ability) && !DisabledAbilities.Contains(Ability))
		{
			EnabledAbilities.Add(Ability);
		}
	}
	
	if (EnabledAbilities.Num() == 0)
	{
		return nullptr;
	}
	
	// Return random enabled ability
	int32 RandomIndex = FMath::RandRange(0, EnabledAbilities.Num() - 1);
	return EnabledAbilities[RandomIndex];
}

UAbilityComponent* UThresholdManager::GetRandomEnabledAbilityExcludingSlash() const
{
	TArray<UAbilityComponent*> EnabledAbilities;
	
	// Find all abilities that aren't disabled and aren't basic abilities
	for (UAbilityComponent* Ability : AllPlayerAbilities)
	{
		if (IsValid(Ability) && !DisabledAbilities.Contains(Ability))
		{
			// Exclude all basic abilities (including slash)
			if (!Ability->IsBasicAbility())
			{
				EnabledAbilities.Add(Ability);
			}
		}
	}
	
	if (EnabledAbilities.Num() == 0)
	{
		return nullptr;
	}
	
	// Return random enabled ability (excluding basic abilities)
	int32 RandomIndex = FMath::RandRange(0, EnabledAbilities.Num() - 1);
	return EnabledAbilities[RandomIndex];
}

// Critical Timer Functions
bool UThresholdManager::IsCriticalTimerActive() const
{
	return bCriticalTimerActive;
}

float UThresholdManager::GetCriticalTimeRemaining() const
{
	if (!bCriticalTimerActive)
	{
		return 0.0f;
	}
	
	float ElapsedTime = GetWorld()->GetTimeSeconds() - CriticalTimerStartTime;
	return FMath::Max(0.0f, CriticalTimerDuration - ElapsedTime);
}

void UThresholdManager::StartCriticalTimer()
{
	if (bCriticalTimerActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Critical timer already active"));
		return;
	}
	
	bCriticalTimerActive = true;
	CriticalTimerStartTime = GetWorld()->GetTimeSeconds();
	
	// Set critical state in ResourceManager to prevent unwanted resets
	if (IsValid(ResourceManager))
	{
		ResourceManager->SetCriticalState(true);
	}
	
	// Activate ultimate mode immediately so abilities are ready
	ActivateUltimateMode();
	
	// Start the death timer
	GetWorld()->GetTimerManager().SetTimer(
		CriticalTimerHandle,
		this,
		&UThresholdManager::OnCriticalTimerExpiredInternal,
		CriticalTimerDuration,
		false
	);
	
	// Start UI update timer (updates every 0.1 seconds)
	GetWorld()->GetTimerManager().SetTimer(
		CriticalUpdateTimerHandle,
		this,
		&UThresholdManager::UpdateCriticalTimer,
		0.1f,
		true
	);
	
	UE_LOG(LogTemp, Error, TEXT("ThresholdManager: CRITICAL ERROR - Player has %.1f seconds to use ultimate ability or DIE!"), CriticalTimerDuration);
	OnCriticalTimer.Broadcast(CriticalTimerDuration);
}

void UThresholdManager::StopCriticalTimer()
{
	if (!bCriticalTimerActive)
	{
		return;
	}
	
	// DEBUG: Log call stack to find what's stopping the critical timer
	const FString CallStack = FFrame::GetScriptCallstack();
	UE_LOG(LogTemp, Error, TEXT("!!! CRITICAL TIMER STOPPED !!! Call stack:\n%s"), *CallStack);
	
	bCriticalTimerActive = false;
	
	// Clear critical state in ResourceManager
	if (IsValid(ResourceManager))
	{
		ResourceManager->SetCriticalState(false);
	}
	
	// Clear both timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CriticalTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(CriticalUpdateTimerHandle);
	}
	
	// Notify UI that timer was stopped
	OnCriticalTimer.Broadcast(0.0f); // Send 0 time remaining
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Critical timer stopped - ultimate ability used in time!"));
}

void UThresholdManager::OnCriticalTimerExpiredInternal()
{
	bCriticalTimerActive = false;
	
	// Clear critical state in ResourceManager
	if (IsValid(ResourceManager))
	{
		ResourceManager->SetCriticalState(false);
	}
	
	// Check if this is the final chance (4th time reaching 100%)
	int32 TimesReached = IsValid(ResourceManager) ? ResourceManager->GetWPMaxReachedCount() : 0;
	if (TimesReached >= GameplayConfig::Thresholds::MAX_WP_REACHES)
	{
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: CRITICAL TIMER EXPIRED ON FINAL CHANCE (%d) - PERMANENT DEATH!"), TimesReached);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: CRITICAL TIMER EXPIRED (%d/%d) - PLAYER DEATH!"), TimesReached, GameplayConfig::Thresholds::MAX_WP_REACHES);
	}
	
	// Broadcast timer expired event
	OnCriticalTimerExpired.Broadcast();
	
	// End combat first to clean up properly
	EndCombat();
	
	// Trigger player death
	OnPlayerDeath.Broadcast();
}

void UThresholdManager::UpdateCriticalTimer()
{
	if (!bCriticalTimerActive)
	{
		return;
	}
	
	float TimeRemaining = GetCriticalTimeRemaining();
	OnCriticalTimer.Broadcast(TimeRemaining);
	
	if (TimeRemaining <= 0.0f)
	{
		// Stop the update timer
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(CriticalUpdateTimerHandle);
		}
	}
}