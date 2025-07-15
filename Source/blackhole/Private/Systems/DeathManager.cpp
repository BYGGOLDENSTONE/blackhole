#include "Systems/DeathManager.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Systems/ResourceManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UDeathManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Set default death conditions
    DeathConditions.MaxAbilityLosses = 3;
    DeathConditions.MaxWPOverloads = 4;
    DeathConditions.bInstantDeathOnMaxAbilities = true;
    DeathConditions.bAllowRevive = false;
    
    // Reset state
    ResetDeathState();
    
    // Cache player reference
    if (UWorld* World = GetWorld())
    {
        PlayerCharacter = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
    }
}

void UDeathManager::Deinitialize()
{
    PlayerCharacter = nullptr;
    Super::Deinitialize();
}

void UDeathManager::CheckDeathConditions()
{
    if (bIsPlayerDead)
    {
        return;
    }
    
    // Check health death
    if (CheckHealthDeath())
    {
        TriggerDeath(EDeathReason::HealthDepleted);
        return;
    }
    
    // Check ability loss death
    if (CheckAbilityLossDeath())
    {
        TriggerDeath(EDeathReason::TooManyAbilitiesLost);
        return;
    }
    
    // Check WP overload death
    if (CheckWPOverloadDeath())
    {
        TriggerDeath(EDeathReason::WPOverload);
        return;
    }
}

void UDeathManager::TriggerDeath(EDeathReason Reason)
{
    if (bIsPlayerDead && !DeathConditions.bAllowRevive)
    {
        return;
    }
    
    LastDeathReason = Reason;
    bIsPlayerDead = true;
    
    ExecuteDeath(Reason);
}

void UDeathManager::IncrementAbilityLosses()
{
    AbilitiesLost++;
    
    // Check if we've hit the death threshold
    if (AbilitiesLost >= DeathConditions.MaxAbilityLosses)
    {
        OnDeathConditionWarning.Broadcast(EDeathReason::TooManyAbilitiesLost);
    }
    
    CheckDeathConditions();
}

void UDeathManager::IncrementWPOverloads()
{
    WPOverloadCount++;
    
    // Check if we've hit the death threshold
    if (WPOverloadCount >= DeathConditions.MaxWPOverloads)
    {
        OnDeathConditionWarning.Broadcast(EDeathReason::WPOverload);
    }
    
    CheckDeathConditions();
}

void UDeathManager::OnHealthDepleted()
{
    TriggerDeath(EDeathReason::HealthDepleted);
}

void UDeathManager::SetDeathConditions(const FDeathConditions& Conditions)
{
    DeathConditions = Conditions;
}

void UDeathManager::ResetDeathState()
{
    bIsPlayerDead = false;
    AbilitiesLost = 0;
    WPOverloadCount = 0;
    LastDeathReason = EDeathReason::None;
}

bool UDeathManager::CheckHealthDeath() const
{
    // In the new energy system, death doesn't occur from WP depletion
    // Death only occurs if critical timer expires without using ultimate
    // This is handled by ThresholdManager, not DeathManager
    return false;
}

bool UDeathManager::CheckAbilityLossDeath() const
{
    // Instant death if we've lost too many abilities and hit 100% WP again
    if (DeathConditions.bInstantDeathOnMaxAbilities && 
        AbilitiesLost >= DeathConditions.MaxAbilityLosses)
    {
        // This death is triggered by ThresholdManager when WP hits 100%
        // So we just check if we're at the threshold
        return false; // Let ThresholdManager handle this
    }
    
    return false;
}

bool UDeathManager::CheckWPOverloadDeath() const
{
    // Death if we've hit 100% WP too many times
    return WPOverloadCount >= DeathConditions.MaxWPOverloads;
}

void UDeathManager::ExecuteDeath(EDeathReason Reason)
{
    // Log death reason
    FString ReasonString;
    switch (Reason)
    {
        case EDeathReason::HealthDepleted:
            ReasonString = TEXT("Health Depleted");
            break;
        case EDeathReason::TooManyAbilitiesLost:
            ReasonString = TEXT("Too Many Abilities Lost");
            break;
        case EDeathReason::WPOverload:
            ReasonString = TEXT("Willpower Overload");
            break;
        case EDeathReason::Environmental:
            ReasonString = TEXT("Environmental");
            break;
        case EDeathReason::Debug:
            ReasonString = TEXT("Debug");
            break;
        default:
            ReasonString = TEXT("Unknown");
            break;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Player Death: %s"), *ReasonString);
    
    // Broadcast death event
    OnPlayerDeath.Broadcast(Reason);
    
    // Tell the player character to die
    if (PlayerCharacter)
    {
        PlayerCharacter->Die();
    }
}