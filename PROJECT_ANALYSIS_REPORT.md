# Blackhole Project - Cleanup Issues Analysis Report

## Executive Summary
This report identifies potential cleanup issues in the blackhole codebase that could cause crashes when restarting the game. Several critical issues were found related to timer cleanup, delegate unbinding, and component access patterns.

## Critical Issues Found

### 1. DataSpikeAbility Component - Timer Cleanup Issues
**File**: `/Source/blackhole/Private/Components/Abilities/Player/Hacker/DataSpikeAbility.cpp`

**Issues Found**:
- The `ActiveDOTs` map contains timer handles that reference enemy actors
- In `EndPlay()`, timers are cleared but enemy actors might already be destroyed
- The `RemoveDOTFromEnemy()` function doesn't validate if the world is still valid before accessing timer manager

**Risk**: HIGH - Could cause crashes if enemies are destroyed before the ability component

**Recommendations**:
1. Add validity checks for GetWorld() in RemoveDOTFromEnemy()
2. Consider using weak object pointers for enemy references in the ActiveDOTs map
3. Clear ActiveDOTs entries when enemies are destroyed (bind to enemy's OnDestroyed delegate)

### 2. SystemOverrideAbility Component - Similar Timer Issues
**File**: `/Source/blackhole/Private/Components/Abilities/Player/Hacker/SystemOverrideAbility.cpp`

**Issues Found**:
- `DisabledEnemies` map has the same pattern as DataSpikeAbility
- Timers reference enemy actors that might be destroyed
- No cleanup when enemies are destroyed while disabled

**Risk**: HIGH - Same crash potential as DataSpikeAbility

**Recommendations**:
1. Implement the same fixes as DataSpikeAbility
2. Add enemy validity checks in RestoreEnemy()
3. Consider a centralized enemy tracking system

### 3. HUD Component References
**File**: `/Source/blackhole/Private/UI/BlackholeHUD.cpp`

**Issues Found**:
- HUD caches many ability component pointers but has no EndPlay() implementation
- No cleanup of delegate bindings to ResourceManager and ThresholdManager
- FindComponentByClass() is called in DrawHUD every frame for some components

**Risk**: MEDIUM - Could cause crashes if HUD persists across level transitions

**Recommendations**:
1. Implement EndPlay() in ABlackholeHUD to unbind delegates:
```cpp
void ABlackholeHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ResourceManager)
    {
        ResourceManager->OnWillPowerChanged.RemoveDynamic(this, &ABlackholeHUD::UpdateWPBar);
        ResourceManager->OnHeatChanged.RemoveDynamic(this, &ABlackholeHUD::UpdateHeatBar);
    }
    
    if (ThresholdManager)
    {
        ThresholdManager->OnUltimateModeActivated.RemoveDynamic(this, &ABlackholeHUD::OnUltimateModeChanged);
    }
    
    Super::EndPlay(EndPlayReason);
}
```

### 4. ResourceManager Cleanup
**File**: `/Source/blackhole/Private/Systems/ResourceManager.cpp`

**Issues Found**:
- Good cleanup in Deinitialize() but delegates might still have bound objects
- Timer cleanup is correct

**Risk**: LOW - Properly implemented cleanup

**Positive**: This is a good example of proper cleanup implementation

### 5. ThresholdManager Cleanup
**File**: `/Source/blackhole/Private/Systems/ThresholdManager.cpp`

**Issues Found**:
- Good cleanup in Deinitialize()
- Properly unbinds from ResourceManager
- Has validity checks for abilities

**Risk**: LOW - Well implemented

**Positive**: Another good example of proper cleanup

### 6. BaseEnemy Cleanup
**File**: `/Source/blackhole/Private/Enemy/BaseEnemy.cpp`

**Issues Found**:
- Properly implements EndPlay() to clear timers
- Good cleanup pattern

**Risk**: LOW - Properly implemented

## General Patterns Observed

### Problematic Patterns:
1. **Timer Delegates with Raw Pointers**: Using BindUObject with enemy references in timers without tracking enemy destruction
2. **Missing EndPlay in UI Classes**: HUD doesn't clean up its delegate bindings
3. **No Weak Object Pointers**: Strong references to actors that might be destroyed

### Good Patterns:
1. **Subsystem Cleanup**: ResourceManager and ThresholdManager properly clean up in Deinitialize()
2. **Validity Checks**: Most code checks IsValid() before using cached pointers
3. **Timer Cleanup**: Most components clear timers in EndPlay()

## Recommended Actions

### Immediate (High Priority):
1. Add EndPlay() to ABlackholeHUD with proper delegate cleanup
2. Modify DataSpikeAbility and SystemOverrideAbility to use weak object pointers for enemy tracking
3. Add enemy destruction callbacks to clean up ability references

### Short Term (Medium Priority):
1. Audit all uses of BindUObject and ensure proper cleanup
2. Create a centralized enemy tracking system to manage ability-enemy relationships
3. Add more defensive GetWorld() validity checks before timer operations

### Long Term (Low Priority):
1. Consider using gameplay tags instead of direct actor references for some systems
2. Implement a more robust ability-target relationship system
3. Add automated tests for level restart scenarios

## Code Snippets for Fixes

### Fix for DataSpikeAbility (High Priority):
```cpp
// In DataSpikeAbility.h
UPROPERTY()
TMap<TWeakObjectPtr<ABaseEnemy>, FDataCorruption> ActiveDOTs;

// In RemoveDOTFromEnemy
void UDataSpikeAbility::RemoveDOTFromEnemy(ABaseEnemy* Enemy)
{
    if (!Enemy)
    {
        return;
    }
    
    TWeakObjectPtr<ABaseEnemy> WeakEnemy(Enemy);
    if (!ActiveDOTs.Contains(WeakEnemy))
    {
        return;
    }
    
    // Clear timer with world validity check
    if (UWorld* World = GetWorld())
    {
        if (ActiveDOTs[WeakEnemy].DOTTimerHandle.IsValid())
        {
            World->GetTimerManager().ClearTimer(ActiveDOTs[WeakEnemy].DOTTimerHandle);
        }
    }
    
    // Remove from tracking
    ActiveDOTs.Remove(WeakEnemy);
}
```

### Fix for HUD (High Priority):
```cpp
// Add to BlackholeHUD.cpp
void ABlackholeHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unbind all delegates
    if (ResourceManager)
    {
        ResourceManager->OnWillPowerChanged.RemoveDynamic(this, &ABlackholeHUD::UpdateWPBar);
        ResourceManager->OnHeatChanged.RemoveDynamic(this, &ABlackholeHUD::UpdateHeatBar);
    }
    
    if (ThresholdManager)
    {
        ThresholdManager->OnUltimateModeActivated.RemoveDynamic(this, &ABlackholeHUD::OnUltimateModeChanged);
    }
    
    // Clear cached pointers
    PlayerCharacter = nullptr;
    ResourceManager = nullptr;
    ThresholdManager = nullptr;
    
    // Clear all cached ability pointers
    CachedSlashAbility = nullptr;
    CachedKillAbility = nullptr;
    CachedFirewallBreach = nullptr;
    CachedPulseHack = nullptr;
    CachedGravityPull = nullptr;
    CachedDataSpike = nullptr;
    CachedSystemOverride = nullptr;
    CachedHackerDash = nullptr;
    CachedHackerJump = nullptr;
    CachedMoltenMace = nullptr;
    CachedHeatShield = nullptr;
    CachedBlastCharge = nullptr;
    CachedHammerStrike = nullptr;
    CachedForgeDash = nullptr;
    CachedForgeJump = nullptr;
    CachedComboComponent = nullptr;
    
    Super::EndPlay(EndPlayReason);
}
```

## Testing Recommendations

1. **Restart Test**: Start game → Enter combat → Use abilities on enemies → Restart level → Verify no crashes
2. **Enemy Destruction Test**: Apply DOT effects → Kill enemies quickly → Verify no crashes
3. **Level Transition Test**: Use abilities → Transition to new level → Verify cleanup
4. **Memory Leak Test**: Profile memory usage across multiple game restarts

## Conclusion

The most critical issues are in the new ability components (DataSpikeAbility and SystemOverrideAbility) and the HUD's missing cleanup. These should be addressed immediately to prevent crashes during game restarts. The subsystems (ResourceManager and ThresholdManager) show good cleanup patterns that should be replicated in other components.