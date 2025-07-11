#include "Systems/BuffManager.h"
#include "Engine/World.h"

void UBuffManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Initialize with empty buffs
    CombinedBuffs.Reset();
    ActiveBuffs.Empty();
}

void UBuffManager::Deinitialize()
{
    ClearAllBuffs();
    Super::Deinitialize();
}

void UBuffManager::AddBuff(const FString& BuffID, const FCombatBuff& Buff)
{
    if (BuffID.IsEmpty())
    {
        return;
    }
    
    ActiveBuffs.Add(BuffID, Buff);
    RecalculateBuffs();
}

void UBuffManager::RemoveBuff(const FString& BuffID)
{
    if (ActiveBuffs.Remove(BuffID) > 0)
    {
        RecalculateBuffs();
    }
}

void UBuffManager::ClearAllBuffs()
{
    ActiveBuffs.Empty();
    CombinedBuffs.Reset();
    OnBuffsChanged.Broadcast(CombinedBuffs);
}

float UBuffManager::CalculateFinalDamage(float BaseDamage) const
{
    return BaseDamage * CombinedBuffs.DamageMultiplier;
}

float UBuffManager::CalculateFinalCooldown(float BaseCooldown) const
{
    return BaseCooldown * (1.0f - CombinedBuffs.CooldownReduction);
}

void UBuffManager::UpdateWPThresholdBuffs(float WPPercentage)
{
    FCombatBuff WPBuff = CreateWPThresholdBuff(WPPercentage);
    
    if (WPPercentage >= 0.5f) // 50% or higher
    {
        AddBuff(TEXT("WPThreshold"), WPBuff);
    }
    else
    {
        RemoveBuff(TEXT("WPThreshold"));
    }
}

void UBuffManager::UpdateLostAbilityBuffs(int32 AbilitiesLost)
{
    if (AbilitiesLost > 0)
    {
        FCombatBuff LostAbilityBuff = CreateLostAbilityBuff(AbilitiesLost);
        AddBuff(TEXT("LostAbilities"), LostAbilityBuff);
    }
    else
    {
        RemoveBuff(TEXT("LostAbilities"));
    }
}

void UBuffManager::RecalculateBuffs()
{
    // Reset to base values
    CombinedBuffs.Reset();
    
    // Combine all active buffs
    for (const auto& BuffPair : ActiveBuffs)
    {
        CombinedBuffs.CombineWith(BuffPair.Value);
    }
    
    // Notify listeners
    OnBuffsChanged.Broadcast(CombinedBuffs);
}

FCombatBuff UBuffManager::CreateWPThresholdBuff(float WPPercentage) const
{
    FCombatBuff Buff;
    
    if (WPPercentage >= 0.5f && WPPercentage < 1.0f) // 50-99%
    {
        Buff.DamageMultiplier = 1.2f; // +20% damage
        Buff.CooldownReduction = 0.15f; // -15% cooldowns
        Buff.AttackSpeedMultiplier = 1.25f; // +25% attack speed
        Buff.BuffSource = TEXT("WP Threshold (Buffed)");
    }
    
    return Buff;
}

FCombatBuff UBuffManager::CreateLostAbilityBuff(int32 AbilitiesLost) const
{
    FCombatBuff Buff;
    
    if (AbilitiesLost > 0)
    {
        // Scaling buffs per ability lost
        Buff.DamageMultiplier = 1.0f + (0.1f * AbilitiesLost); // +10% per ability
        Buff.CooldownReduction = 0.05f * AbilitiesLost; // -5% per ability
        Buff.AttackSpeedMultiplier = 1.0f + (0.1f * AbilitiesLost); // +10% per ability
        Buff.BuffSource = FString::Printf(TEXT("Survivor (%d abilities lost)"), AbilitiesLost);
    }
    
    return Buff;
}