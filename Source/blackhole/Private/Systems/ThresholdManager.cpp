#include "Systems/ThresholdManager.h"
#include "Systems/ResourceManager.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Components/Abilities/UtilityAbility.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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
	if (ResourceManager)
	{
		ResourceManager->OnWillPowerThresholdReached.RemoveDynamic(this, &UThresholdManager::OnWPThresholdChanged);
		ResourceManager->OnWPMaxReached.RemoveDynamic(this, &UThresholdManager::OnWPMaxReachedHandler);
	}
	
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
	
	// Re-enable all abilities
	for (UAbilityComponent* Ability : DisabledAbilities)
	{
		if (IsValid(Ability))
		{
			Ability->SetComponentTickEnabled(true);
		}
	}
	
	// Reset ultimate mode for all abilities
	for (UAbilityComponent* Ability : AllPlayerAbilities)
	{
		if (IsValid(Ability))
		{
			Ability->SetUltimateMode(false);
		}
	}
	
	DisabledAbilities.Empty();
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
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: WP reached 100%% (Count: %d) - ULTIMATE MODE ACTIVATED!"), TimesReached);
	
	// Check if player has already lost 3 abilities - trigger death
	if (DisabledAbilities.Num() >= 3)
	{
		OnPlayerDeath.Broadcast();
		UE_LOG(LogTemp, Error, TEXT("ThresholdManager: Player death triggered - WP reached 100%% after losing %d abilities!"), DisabledAbilities.Num());
		return;
	}
	
	// Also check if this is the 4th time overall - trigger death
	if (TimesReached >= 4)
	{
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
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Activating ultimate mode, checking %d abilities"), AllPlayerAbilities.Num());
	
	// Set all non-basic abilities to ultimate mode
	int32 UltimateCount = 0;
	for (UAbilityComponent* Ability : AllPlayerAbilities)
	{
		if (IsValid(Ability) && !DisabledAbilities.Contains(Ability))
		{
			// Skip basic abilities - they don't have ultimate versions
			if (!Ability->IsBasicAbility())
			{
				Ability->SetUltimateMode(true);
				UltimateCount++;
				UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Set %s to ultimate mode"), *Ability->GetName());
			}
			else
			{
				// Skipped basic ability - no need to log every time
			}
		}
		// Skip disabled abilities - no need to log
	}
	
	OnUltimateModeActivated.Broadcast(true);
	
	UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: ULTIMATE MODE ACTIVATED - %d abilities enhanced!"), UltimateCount);
}

void UThresholdManager::DeactivateUltimateMode(UAbilityComponent* UsedAbility)
{
	if (!bUltimateModeActive)
	{
		return;
	}
	
	// Basic abilities don't trigger deactivation
	if (UsedAbility && UsedAbility->IsBasicAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: Basic ability used during ultimate mode - no deactivation"));
		return;
	}
	
	bUltimateModeActive = false;
	
	// Deactivate ultimate mode for all abilities
	for (UAbilityComponent* Ability : AllPlayerAbilities)
	{
		if (IsValid(Ability))
		{
			Ability->SetUltimateMode(false);
		}
	}
	
	// Disable the ability that was used (if it's not basic)
	if (UsedAbility && !DisabledAbilities.Contains(UsedAbility) && !UsedAbility->IsBasicAbility())
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
	if (ResourceManager)
	{
		ResourceManager->ResetWPAfterMax();
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
		CurrentBuff.DamageMultiplier = 1.2f; // 20% damage increase at 50% WP
		CurrentBuff.CooldownReduction = 0.15f; // 15% cooldown reduction at 50% WP
		CurrentBuff.AttackSpeed = 1.25f; // 25% faster attack speed at 50% WP
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
		CurrentBuff.DamageMultiplier += (0.1f * DisabledCount); // +10% per lost ability
		CurrentBuff.CooldownReduction += (0.05f * DisabledCount); // +5% per lost ability
		CurrentBuff.AttackSpeed += (0.1f * DisabledCount); // +10% per lost ability
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