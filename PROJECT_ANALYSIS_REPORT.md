# Blackhole Project - Analysis Report

## Executive Summary
This report documents the blackhole codebase architecture and recent major refactoring efforts. The project has undergone significant architectural changes to improve modularity, remove unused features, and focus on a streamlined gameplay experience.

## Recent Major Refactoring (2025-07-10)

### Overview
The project underwent a complete overhaul to simplify the codebase and improve stability:

1. **Complete Forge Path Removal**: All Forge abilities, heat system, and related code eliminated
2. **Combo System Overhaul**: Removed old centralized system, implemented component-based architecture
3. **Critical Bug Fixes**: Fixed stack overflow, access violations, and cooldown persistence issues
4. **Blueprint Integration**: All ability and combo parameters are now Blueprint-editable
5. **UI System Removal**: Temporarily removed for future reimplementation

### Architectural Changes

#### 1. Player Character Simplification
- Removed all Forge-specific abilities (Forge Dash, Forge Jump, Heat Shield, etc.)
- Removed Heat component and related mechanics
- Removed weapon switching logic (only Katana remains)
- Streamlined to single-path character focused on Hacker abilities

#### 2. Component-Based Combo System
- Created `UComboAbilityComponent` base class for all combos
- Individual combo components: `UDashSlashCombo`, `UJumpSlashCombo`
- All combo parameters exposed to Blueprint for designer tweaking
- Removed hardcoded combo logic from ability components

#### 3. Completely Removed Systems
- **Heat System**: HeatComponent files deleted, all heat functions removed from ResourceManager
- **Old Combo System**: ComboTracker, ComboComponent files deleted
- **Forge Abilities**: All Forge ability files deleted (HeatShield, MoltenMaceSlash, HammerStrike, BlastCharge)
- **Forge Utilities**: ForgeDash and ForgeJump deleted
- **UI Components**: All UI/menu implementations (MainMenuWidget, PauseMenuWidget, etc.)
- **Slash+Slash Combo**: BladeDance combo removed as requested

### Current Architecture

#### Core Systems
- **Resource System**: Dual resources - Stamina (consumption) + Willpower (corruption buildup)
- **Ability System**: Component-based with full Blueprint parameter exposure
- **Combo System**: Simple last-input tracking in player character + individual combo components
  - DashSlashCombo (Phantom Strike): Teleport backstab triggered by Dash→Slash within 0.5s
  - JumpSlashCombo (Aerial Rave): Shockwave slam triggered by Jump→Slash within 0.3s
  - Real-time based time slow using FPlatformTime::Seconds() for accurate duration
  - HitStop disabled during combos to avoid time dilation conflicts
- **Death System**: Threshold-based with WP mechanics
- **Input System**: Enhanced input with ability slot mapping

#### Fixed Issues
- **Stack Overflow**: Removed recursive Execute() calls and excessive logging
- **Access Violations**: Added null checks, removed ThresholdManager dependencies
- **Cooldown Persistence**: Added proper EndPlay cleanup
- **Time Slow Duration**: Implemented FPlatformTime::Seconds() for real-time tracking
- **Compilation Errors**: Removed all references to deleted ComboComponent and heat system
- **ComboSystem Type Mismatch**: Fixed incorrect usage as WorldSubsystem
- **Time Slow Conflicts**: Resolved HitStop/combo time slow conflicts by disabling HitStop in combos
- **Time Slow Duration**: Fixed using FPlatformTime::Seconds() for real-time tracking instead of game time

#### Component Hierarchy
```
UActorComponent
├── UAbilityComponent (Base - no heat references)
│   ├── Basic Abilities (Always available)
│   │   ├── USlashAbilityComponent (heat stats removed)
│   │   ├── UKillAbilityComponent (debug ability)
│   │   ├── UHackerDashAbility
│   │   └── UHackerJumpAbility
│   ├── Hacker Combat Abilities
│   │   ├── UFirewallBreachAbility
│   │   ├── UPulseHackAbility
│   │   ├── UGravityPullAbilityComponent
│   │   ├── UDataSpikeAbility
│   │   └── USystemOverrideAbility
│   └── UComboAbilityComponent (Base for combos)
│       ├── UDashSlashCombo (Phantom Strike - teleport backstab)
│       └── UJumpSlashCombo (Aerial Rave - shockwave slam)
└── Attribute Components
    ├── UIntegrityComponent (health)
    ├── UStaminaComponent (action resource)
    └── UWillPowerComponent (corruption meter)
```

#### Combo Detection System
The combo system uses a simple but effective approach:
- Player character tracks `LastAbilityUsed` enum (None/Dash/Jump/Slash)
- Player character tracks `LastAbilityTime` using world time
- When slash is used, it checks if a dash or jump was used within the combo window
- If a valid combo is detected, the appropriate combo component executes instead of normal slash
- All timing parameters are Blueprint-editable on the combo components

#### Removed Components
- HeatComponent (entire heat system)
- ComboTracker (old combo tracking)
- ComboComponent (old combo system)
- All Forge abilities and utilities


