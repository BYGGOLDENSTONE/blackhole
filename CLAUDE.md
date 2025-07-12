# Blackhole Project - Claude Assistant Reference

**Engine**: Unreal Engine 5.5  
**Language**: C++ with Blueprint integration  
**Genre**: Cyberpunk action game with risk/reward combat  
**Repository**: https://github.com/BYGGOLDENSTONE/blackhole  
**Last Updated**: 2025-07-12

## 🚨 Critical Issues to Fix

### 1. Menu System Broken (HIGH PRIORITY)
**File**: `BlackholePlayerCharacter.cpp`  
**Issue**: MenuToggleAction never bound in SetupPlayerInputComponent()  
**Fix**:
```cpp
if (MenuToggleAction)
{
    EnhancedInputComponent->BindAction(MenuToggleAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::ToggleMenu);
}
```

### 2. Memory Management Risks
- **ObjectPoolSubsystem**: Raw AActor* pointers need UPROPERTY
- **ComboDetectionSubsystem**: TMap never cleaned, causing memory leaks
- **ThresholdManager**: Ability pointers lack proper cleanup

### 3. GameMode Initialization Error
**File**: `BlackholeGameMode.cpp`  
**Current**: `DefaultPawnClass = ABlackholePlayerCharacter::StaticClass();`  
**Should be**: Use TSubclassOf<> pattern with EditDefaultsOnly

### 4. Include Path Error
**File**: `HackableObject.h` Line 5  
**Current**: `#include "../Components/Interaction/HackableComponent.h"`  
**Should be**: `#include "Components/Interaction/HackableComponent.h"`

## 🎮 Game Overview

**Core Concept**: Digital warrior in cyberspace managing corruption (Willpower/WP) while using abilities. At 100% WP, abilities become ultimates - using one permanently disables it.

**Death Conditions**:
1. Health reaches 0
2. WP reaches 100% after losing 3 abilities  
3. WP reaches 100% for the 4th time overall

## ⚡ Quick Ability Reference

| Ability | Key | WP Change | Type | Status |
|---------|-----|-----------|------|---------|
| **Katana Slash** | LMB | +2 | Basic (Always Available) | ✅ |
| **Hacker Dash** | Shift | 0 | Basic (Always Available) | ✅ |
| **Hacker Jump** | Space | 0 | Basic (Always Available) | ✅ |
| **Firewall Breach** | RMB | +15 | Combat (Can Sacrifice) | ✅ |
| **Pulse Hack** | Q | +20 | Combat (Can Sacrifice) | ✅ |
| **Gravity Pull** | E | +15 | Combat (Can Sacrifice) | ✅ |
| **Data Spike** | R | +25 | Combat (Can Sacrifice) | ✅ |
| **System Override** | F | +30 | Combat (Can Sacrifice) | ✅ |

**Combos**:
- **Phantom Strike**: Dash → Slash (Teleport backstab)
- **Aerial Rave**: Jump → Slash (Ground slam)

## 🏗️ Architecture Quick Reference

### Core Systems
```
ResourceManager (GameInstance) → Manages WP only (Stamina removed)
ThresholdManager (World) → Handles WP thresholds and ultimate mode
BuffManager (World) → Combat buffs at 50%+ WP
DeathManager (World) → Death condition tracking
ComboDetectionSubsystem (World) → Input-based combos
ObjectPoolSubsystem (World) → Performance optimization
GameStateManager (GameInstance) → Menu/pause/play states
```

### Key Files
- **Player**: `BlackholePlayerCharacter.h/cpp`
- **Abilities**: `Components/Abilities/Player/Hacker/*`
- **Resources**: `Systems/ResourceManager.h/cpp`
- **Config**: `Config/GameplayConfig.h` (all constants)
- **GameMode**: `Core/BlackholeGameMode.h/cpp`

## ⚠️ Important Development Notes

### Resource System Confusion
**Issue**: WP costs actually ADD corruption (not consume)  
**Reason**: Intentional corruption mechanic  
**TODO**: Rename functions or document clearly

### Build Process
- **NEVER** build from terminal
- Use Unreal Editor only
- Provide code fixes, let user compile

### Console Commands
```
SetWP <0-100>          # Set Willpower
ForceUltimateMode      # Activate ultimates
ForceAbilityLoss <n>   # Disable abilities
StartCombat            # Test combat state
ShowDebugInfo          # Debug display
```

### Repository Management
**When user says "push the project"**:
```bash
cd "D:\Unreal Projects\blackhole"
git add .
git commit -m "claude auto"
git push origin main
```
**Repository**: https://github.com/BYGGOLDENSTONE/blackhole

## 🔧 Current Implementation Status

### ✅ Working
- 11 abilities with ultimate forms
- WP corruption system
- Death conditions
- 2 combos (Phantom Strike, Aerial Rave)
- Basic enemy AI (4 types)
- Menu system (once input binding fixed)
- Full Blueprint integration

### ❌ Known Issues
- Menu input binding missing
- Memory leaks in subsystems
- AI doesn't use proper UE5 patterns (no AIController/BehaviorTree)
- Significant code duplication in enemies
- HUD performance (calculates every frame)

### 🔄 In Progress
- Environmental interactions
- Additional combos
- VFX/SFX polish

## 📊 Code Quality Summary

**Overall Score**: 7.5/10

**Good**:
- Clean component architecture
- Excellent subsystem usage
- Data-driven combo system
- Good delegate patterns

**Needs Work**:
- Memory management
- AI architecture
- Performance optimization
- Code duplication

## 🚀 Quick Start Guide

### Adding New Ability
1. Inherit from `UAbilityComponent`
2. Override `Execute()` and `ExecuteUltimate()`
3. Add to player Blueprint
4. Set properties in editor

### Testing Death System
```
SetWP 100              # Trigger ultimate mode
ForceAbilityLoss 3     # Lose 3 abilities
SetWP 100              # Instant death
```

### Creating Combos
1. Create `UComboDataAsset` in editor
2. Define input sequence
3. Link combo component
4. Auto-detected by system

## 📁 Project Structure
```
blackhole/
├── Source/
│   ├── Components/Abilities/  # All abilities
│   ├── Systems/              # Core subsystems
│   ├── Player/              # Player character
│   ├── Enemy/               # Enemy types
│   └── UI/                  # HUD and menus
├── Config/GameplayConfig.h   # All constants
└── Content/                  # UE5 assets
```

## 🎯 Priority Fixes

### Immediate (Do First)
1. Add MenuToggleAction binding
2. Fix GameMode initialization
3. Add UPROPERTY to raw pointers
4. Fix include path in HackableObject.h

### Short-term
1. Document WP corruption clearly
2. Extract common enemy code
3. Implement proper widget cleanup
4. Cache HUD calculations

### Long-term
1. Implement AIControllers
2. Add Gameplay Tags
3. Create AI LOD system
4. Setup replication

---

**Reference**: For detailed game design, see `GDD.md`