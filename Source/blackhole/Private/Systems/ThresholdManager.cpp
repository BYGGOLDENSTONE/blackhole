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
		}
	}
}

void UThresholdManager::Deinitialize()
{
	// Unbind from resource manager
	if (ResourceManager)
	{
		ResourceManager->OnWillPowerThresholdReached.RemoveDynamic(this, &UThresholdManager::OnWPThresholdChanged);
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
	
	// Re-enable all abilities
	for (UAbilityComponent* Ability : DisabledAbilities)
	{
		if (IsValid(Ability))
		{
			Ability->SetComponentTickEnabled(true);
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
	
	int32 AbilitiesToDisable = 0;
	
	// Check which thresholds to trigger based on current WP%
	switch (NewThreshold)
	{
		case EResourceThreshold::Warning: // 30-60%
			if (!ThresholdState.bMediumTriggered)
			{
				ThresholdState.bMediumTriggered = true;
				AbilitiesToDisable = 1;
			}
			break;
			
		case EResourceThreshold::Critical: // 10-30%
			if (!ThresholdState.bLowTriggered)
			{
				ThresholdState.bLowTriggered = true;
				AbilitiesToDisable = 1; // Disable one more (total 2)
			}
			break;
			
		case EResourceThreshold::Emergency: // <10%
			if (!ThresholdState.bCriticalTriggered)
			{
				ThresholdState.bCriticalTriggered = true;
				AbilitiesToDisable = 1; // Disable one more (total 3)
			}
			break;
			
		default:
			break;
	}
	
	// Disable abilities if threshold was triggered
	if (AbilitiesToDisable > 0)
	{
		DisableRandomAbilities(AbilitiesToDisable);
		UpdateSurvivorBuffs();
	}
}

void UThresholdManager::DisableRandomAbilities(int32 NumberToDisable)
{
	for (int32 i = 0; i < NumberToDisable; i++)
	{
		UAbilityComponent* AbilityToDisable = GetRandomEnabledAbility();
		if (!AbilityToDisable)
		{
			UE_LOG(LogTemp, Warning, TEXT("ThresholdManager: No more abilities to disable"));
			break;
		}
		
		// Disable the ability
		AbilityToDisable->SetComponentTickEnabled(false);
		DisabledAbilities.Add(AbilityToDisable);
		
		// Broadcast event
		OnAbilityDisabled.Broadcast(AbilityToDisable, DisabledAbilities.Num());
		
		UE_LOG(LogTemp, Log, TEXT("ThresholdManager: Disabled ability %s (Total disabled: %d)"), 
			*AbilityToDisable->GetName(), DisabledAbilities.Num());
	}
}

void UThresholdManager::UpdateSurvivorBuffs()
{
	int32 DisabledCount = DisabledAbilities.Num();
	
	// Calculate buffs based on disabled abilities
	CurrentBuff.DamageMultiplier = 1.0f + (0.25f * DisabledCount);
	CurrentBuff.CooldownReduction = 0.15f * DisabledCount;
	CurrentBuff.NextCastWPRefund = 5 * DisabledCount;
	
	// Broadcast buff update
	OnSurvivorBuff.Broadcast(CurrentBuff);
	
	UE_LOG(LogTemp, Log, TEXT("ThresholdManager: Survivor buffs updated - Damage: %.0f%%, Cooldown: -%.0f%%, WP Refund: +%d"), 
		CurrentBuff.DamageMultiplier * 100.0f, 
		CurrentBuff.CooldownReduction * 100.0f,
		CurrentBuff.NextCastWPRefund);
}

void UThresholdManager::CachePlayerAbilities()
{
	AllPlayerAbilities.Empty();
	
	if (!PlayerCharacter)
	{
		return;
	}
	
	// Get all ability components on player
	TArray<UActorComponent*> Components = PlayerCharacter->GetComponents().Array();
	for (UActorComponent* Component : Components)
	{
		if (UAbilityComponent* Ability = Cast<UAbilityComponent>(Component))
		{
			// Skip utility abilities - they should never be disabled
			if (Cast<UUtilityAbility>(Ability))
			{
				continue;
			}
			
			AllPlayerAbilities.Add(Ability);
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("ThresholdManager: Cached %d player abilities"), AllPlayerAbilities.Num());
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