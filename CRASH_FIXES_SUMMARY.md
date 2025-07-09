# Crash Fixes Summary - 2025-07-09

## Issues Fixed
1. Game crashes with access violation (0xc0000005) when restarting after dying at 100% WP
2. Intermittent crashes during ability usage
3. Access violations when transitioning between game states

## Root Causes Identified
1. **Enemy Tracking with Raw Pointers**: DataSpikeAbility and SystemOverrideAbility were tracking enemies with raw pointers in timers, causing crashes when enemies were destroyed
2. **Missing HUD Cleanup**: BlackholeHUD had no EndPlay implementation to unbind delegates
3. **Dead Player State**: Abilities could still execute on dead players when restarting

## Fixes Applied

### 1. DataSpikeAbility.h/cpp
- Changed `TMap<ABaseEnemy*, FDataCorruption>` to `TMap<TWeakObjectPtr<ABaseEnemy>, FDataCorruption>`
- Updated all functions to use weak pointers
- Added world validity checks before timer operations

### 2. SystemOverrideAbility.h/cpp
- Changed `TMap<ABaseEnemy*, FSystemDisable>` to `TMap<TWeakObjectPtr<ABaseEnemy>, FSystemDisable>`
- Updated DisableEnemy, RestoreEnemy, and EndPlay to handle weak pointers
- Added cleanup for invalid enemy references

### 3. BlackholeHUD.h/cpp
- Added `virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;`
- Implemented proper delegate unbinding for ResourceManager and ThresholdManager
- Clear all cached ability pointers on EndPlay

### 4. BlackholePlayerCharacter.cpp
- Added `bIsDead = false;` at the beginning of BeginPlay to reset death state
- Ensures clean restart when playing again after death

### 5. AbilityComponent.cpp
- Added death check in CanExecute() to prevent abilities from executing on dead players
- Added include for BlackholePlayerCharacter.h

## Testing Recommendations
1. Play game and use all abilities
2. Die at 100% WP three times
3. Press ESC when "YOU DIED!" appears
4. Press Play again in editor
5. Verify no crash occurs and game restarts properly

## Additional Fixes (Part 2)

### 6. HeatShieldAbility & BlastChargeAbility Timer Cleanup
- Added EndPlay methods to clear timers properly
- Clean up particle effects on destruction
- Restore player movement speed if BlastCharge destroyed while charging

### 7. GameStateManager Creation
- New centralized game state management system
- Handles transitions: MainMenu → Playing → GameOver → Reset
- Ensures proper cleanup of all systems during transitions
- Prevents race conditions between state changes

### 8. ThresholdManager Improvements
- Added CleanupInvalidAbilities() calls to critical methods
- Prevents accessing destroyed ability pointers
- More frequent cleanup of stale references

## Additional Fixes (Part 3 - Afternoon Session)

### 9. Pointer Safety Improvements
- Fixed unsafe GetOwner() calls without null checks in multiple components
- Fixed unsafe GetWorld() calls in UI code  
- Added null checks before all pointer dereferences
- Fixed brace mismatch error introduced during safety fixes

#### Files Fixed:
- **HackableComponent.cpp**: Fixed 2 instances of GetOwner()->GetName()
- **GravityPullAbilityComponent.cpp**: Fixed GetOwner()->GetActorLocation()  
- **SlashAbilityComponent.cpp**: Improved GetOwner() usage in BeginPlay
- **SimplePauseMenu.cpp**: Fixed 3 GetWorld()->GetFirstPlayerController() calls

## Key Patterns Fixed
- **Weak Pointers**: Use `TWeakObjectPtr<>` for any objects tracked in timers or delegates
- **World Validity**: Always check `if (UWorld* World = GetWorld())` before timer operations
- **Delegate Cleanup**: Always unbind delegates in EndPlay/Deinitialize
- **State Reset**: Reset game state flags in BeginPlay for proper restart
- **Timer Cleanup**: Always implement EndPlay for components with timers
- **Centralized State Management**: Use GameStateManager for all state transitions
- **Null Checks**: Always check GetOwner() and GetWorld() return values before using
- **Safe Component Access**: Use if (Component = FindComponentByClass<>()) pattern