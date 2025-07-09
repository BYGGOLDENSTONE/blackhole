#include "Systems/ThresholdManager.h"
#include "Systems/ResourceManager.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Components/Abilities/UtilityAbility.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Config/GameplayConfig.h"

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
	
	// Clear all delegate bindings to prevent crashes
	OnCombatStarted.Clear();
	OnCombatEnded.Clear();
	OnAbilityDisabled.Clear();
	OnSurvivorBuff.Clear();
	OnUltimateModeActivated.Clear();
	OnPlayerDeath.Clear();
	
	// Clear ability arrays
	AllPlayerAbilities.Empty();
	DisabledAbilities.Empty();
	
	Super::Deinitialize();
}

void UThresholdManager::StartCombat()
{
	if (bIsInCombat)
	{
		return;
	}
	
	bIsInCombat = true;
	
	// Reset threshold state for new combat
	ThresholdState.Reset();
	DisabledAbilities.Empty();
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
	
	bIsInCombat = false;
	bUltimateModeActive = false;
	
	// Clean up invalid abilities before processing
	CleanupInvalidAbilities();
	
	// Clear all timers for this object
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	
	// Re-enable all abilities
	for (int32 i = DisabledAbilities.Num() - 1; i >= 0; i--)
	{
		UAbilityComponent* Ability = DisabledAbilities[i];
		if (IsValid(Ability))
		{
			Ability->SetComponentTickEnabled(true);
		}
		else
		{
			DisabledAbilities.RemoveAt(i);
		}
	}
	
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
	
	DisabledAbilities.Empty();
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
	if (!bIsInCombat)
	{
		return;
	}
	
	// Clean up invalid abilities before processing
	CleanupInvalidAbilities();
	
	// Ensure ResourceManager is valid
	if (!IsValid(ResourceManager))
	{
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: ResourceManager is null in OnWPMaxReachedHandler!"));
		return;
	}
	
	// Only apply ultimate mode for Hacker path
	if (ResourceManager->GetCurrentPath() != ECharacterPath::Hacker)
	{
		UE_LOG(LogTemp, Log, TEXT("ThresholdManager: WP reached 100%% but player is on Forge path - ignoring"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: WP reached 100%% (Count: %d) - ULTIMATE MODE ACTIVATED!"), TimesReached);
	
	// Check if player has already lost 3 abilities - trigger death
	if (DisabledAbilities.Num() >= GameplayConfig::Thresholds::MAX_DISABLED_ABILITIES)
	{
		// End combat first to clean up properly
		EndCombat();
		OnPlayerDeath.Broadcast();
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: Player death triggered - WP reached 100%% after losing %d abilities!"), DisabledAbilities.Num());
		return;
	}
	
	// Also check if this is the max times overall - trigger death
	if (TimesReached >= GameplayConfig::Thresholds::MAX_WP_REACHES)
	{
		// End combat first to clean up properly
		EndCombat();
		OnPlayerDeath.Broadcast();
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: Player death triggered - WP reached 100%% for the 4th time!"));
		return;
	}
	
	// Activate ultimate mode for all abilities
	ActivateUltimateMode();
}

void UThresholdManager::ActivateUltimateMode()
{
	if (bUltimateModeActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Ultimate mode already active"));
		return;
	}
	
	bUltimateModeActive = true;
	
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
				UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Set %s to ultimate mode"), *Ability->GetName());
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
		return;
	}
	
	// Basic abilities don't trigger deactivation
	if (IsValid(UsedAbility) && UsedAbility->IsBasicAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Basic ability used during ultimate mode - no deactivation"));
		return;
	}
	
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
		UsedAbility->SetComponentTickEnabled(false);
		DisabledAbilities.Add(UsedAbility);
		
		// Update buffs for lost abilities
		UpdateSurvivorBuffs();
		
		// Broadcast event
		OnAbilityDisabled.Broadcast(UsedAbility, DisabledAbilities.Num());
		
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Disabled ability %s after ultimate use (Total disabled: %d)"), 
			*UsedAbility->GetName(), DisabledAbilities.Num());
	}
	
	// Reset WP to 0
	if (IsValid(ResourceManager))
	{
		ResourceManager->ResetWPAfterMax();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: ResourceManager is null in DeactivateUltimateMode!"));
	}
	
	OnUltimateModeActivated.Broadcast(false);
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Ultimate mode deactivated"));
}

void UThresholdManager::OnAbilityExecuted(UAbilityComponent* Ability)
{
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: OnAbilityExecuted called for %s (UltimateMode: %s, IsBasic: %s)"), 
		Ability ? *Ability->GetName() : TEXT("NULL"),
		bUltimateModeActive ? TEXT("YES") : TEXT("NO"),
		Ability && Ability->IsBasicAbility() ? TEXT("YES") : TEXT("NO"));
	
	// If in ultimate mode and a non-basic ability was used, deactivate and disable it
	if (bUltimateModeActive && Ability && !Ability->IsBasicAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Deactivating ultimate mode after using %s"), *Ability->GetName());
		DeactivateUltimateMode(Ability);
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