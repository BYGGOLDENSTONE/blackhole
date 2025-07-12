# Blackhole - Hacker Path Abilities Documentation
**Engine**: Unreal Engine 5.5  
**Last Updated**: 2025-07-11  
**Architecture**: Custom C++ Component System (No GAS)

## 📋 Table of Contents
1. [System Overview](#-system-overview)
2. [Abilities Quick Reference](#-abilities-quick-reference)
3. [Basic Abilities](#-basic-abilities)
4. [Combat Abilities](#-combat-abilities)
5. [Combo Abilities](#-combo-abilities)
6. [Ultimate System](#-ultimate-system)
7. [Implementation Architecture](#-implementation-architecture)
8. [Blueprint Integration](#-blueprint-integration)

---

## 🎯 System Overview

### Core Concepts
- **Single Resource System**: Willpower (WP) only - Stamina has been removed
- **Single Character Path**: Hacker (focuses on digital warfare and agility)
- **Component-Based Abilities**: Each ability is a self-contained component
- **Blueprint Editable**: All ability parameters can be tweaked in editor
- **Ultimate System**: At 100% WP, abilities transform into powerful ultimates
- **Strategic Sacrifice**: Using an ultimate permanently disables that ability
- **Death Conditions**: 
  - Integrity reaches 0 (damage)
  - WP reaches 100% after losing 3 abilities
  - WP reaches 100% for the 4th time overall

### Resource Mechanics - Willpower (WP)
- **0-49% WP**: Normal state
- **50-89% WP**: Buffed state (+20% damage, -15% cooldowns, +25% attack speed)
- **90-99% WP**: WARNING - Advanced abilities may be blocked
- **100% WP**: Ultimate mode activated (basic abilities still work)
- **Corruption**: Using abilities ADDS WP (bad)
- **Cleansing**: Some abilities remove WP (good)
- **Basic Ability Exception**: Slash, Dash, Jump always work regardless of WP level

---

## ⚡ Abilities Quick Reference

| Ability | Key | Type | Target | Cooldown | WP Cost | Status |
|---------|-----|------|--------|----------|---------|---------|
| **Katana Slash** | LMB | Melee | Front | 0.3s | +2 | ✅ (Basic) |
| **Hacker Dash** | Shift | Movement | Self | 1s | 0 | ✅ (Basic) |
| **Hacker Jump** | Space | Movement | Self | 0.5s* | 0 | ✅ (Basic) |
| **Firewall Breach** | RMB | Debuff | Single | 4s | +15 | ✅ |
| **Pulse Hack** | Q | AoE Slow | Area | 6s | +20 | ✅ |
| **Gravity Pull** | E | Pull | Target | 5s | +15 | ✅ |
| **Data Spike** | R | Projectile | Target | 4s | +25 | ✅ |
| **System Override** | F | Ultimate | AoE | 90s | +30 | ✅ |
| **Kill (Debug)** | K | Instant Kill | Closest | 5s | 0 | ✅ |

**Legend**:
- ✅ = Implemented and functional
- (Basic) = Always available, cannot be disabled/sacrificed
- *0.5s = Jump-to-jump cooldown (not ability cooldown)

---

## 🛡️ Basic Abilities

Basic abilities are always available and never disabled by WP levels or ultimate sacrifices.

### Katana Slash (LMB)
- **Type**: Melee attack
- **Damage**: 20 base (affected by buffs)
- **Range**: 200 units
- **Combo**: 3-hit sequence with increasing damage
- **WP Generation**: +2 per hit
- **Special**: Can trigger combo abilities when combined with Dash or Jump

### Hacker Dash (Shift)
- **Type**: Directional dash based on movement input
- **Distance**: 500 units
- **WP Cost**: 0
- **Cooldown**: 1 second
- **Special Features**:
  - Dashes in the direction of movement input (WASD)
  - W+Dash: Special case - follows camera angle for vertical movement
  - Phase through enemies
  - Brief invulnerability frames
  - Can combo with Slash for "Phantom Strike"

### Hacker Jump (Space)
- **Type**: Enhanced double jump
- **Jump Height**: Variable based on hold time
- **WP Cost**: 0
- **Air Control**: Enhanced air movement
- **Special Features**:
  - Double jump capability
  - Can combo with Slash for "Aerial Rave"
  - Wall jump potential (if near walls)

---

## ⚔️ Combat Abilities

### Firewall Breach (RMB)
- **Type**: Digital debuff
- **Range**: 800 units
- **Effect**: Marks enemy for increased damage (+30%)
- **Duration**: 5 seconds
- **WP Cost**: +15
- **Cooldown**: 4 seconds
- **Visual**: Red digital corruption effect on target

### Pulse Hack (Q)
- **Type**: AoE crowd control
- **Radius**: 400 units
- **Effect**: 50% slow for 3 seconds
- **Damage**: 30 pulse damage
- **WP Cost**: +20
- **Cooldown**: 6 seconds
- **Special**: Disrupts enemy AI targeting briefly

### Gravity Pull (E)
- **Type**: Single target displacement
- **Range**: 1000 units
- **Pull Force**: 1500 units/second
- **Duration**: 1.5 seconds
- **WP Cost**: +15
- **Cooldown**: 5 seconds
- **Combo Potential**: Sets up for melee combos

### Data Spike (R)
- **Type**: High-damage projectile
- **Damage**: 80 base
- **Range**: 1200 units
- **Projectile Speed**: 2000 units/second
- **WP Cost**: +25
- **Cooldown**: 4 seconds
- **Special**: Pierces through shields

### System Override (F)
- **Type**: Ultimate AoE control
- **Requirements**: 100% WP to activate
- **Effect**: Stuns all enemies in 600 unit radius for 3 seconds
- **Damage**: 150 to all affected
- **Cooldown**: 90 seconds
- **Sacrifice**: Permanently disables this ability after use
- **Visual**: Matrix-like digital rain effect

---

## 🎯 Combo Abilities

Combo abilities are executed by specific input sequences and use the component-based system with advanced targeting.

### Phantom Strike (Dash + Slash)
- **Component**: `UDashSlashCombo`
- **Input Window**: 0.5 seconds
- **Effect**: Teleport behind enemy for backstab
- **Damage**: 150% slash damage × 2.0 backstab multiplier
- **Targeting System**:
  - **Ground Mode**: Prioritizes closest enemy to prevent targeting past enemies
  - **Air Mode**: Uses sophisticated scoring (angle 50% + vertical 30% + distance 20%)
  - **Aim Forgiveness**: 180-unit radius sphere for easier targeting
- **Blueprint Parameters**:
  - Teleport Distance (150 units)
  - Backstab Multiplier (2.0x)
  - Time Slow Scale (0.1)
  - Auto-rotate to target (true)
  - Aim Forgiveness Radius (10.0-300.0 units)

### Aerial Rave (Jump + Slash)
- **Component**: `UJumpSlashCombo`
- **Input Window**: 0.3 seconds while airborne
- **Effect**: Downward slash with shockwave
- **Damage**: 125% slash + 50 shockwave damage
- **Targeting System**:
  - **Ground Mode**: Direct crosshair trace with closest enemy fallback
  - **Air Mode**: Sophisticated scoring for fluid aerial combat
  - **Aim Forgiveness**: 200-unit radius sphere
- **Blueprint Parameters**:
  - Shockwave Radius (300 units)
  - Shockwave Damage (50)
  - Downward Force (1000)
  - Aim Forgiveness Radius (10.0-300.0 units)
  - Damage falloff with distance

### Combo Targeting Features
- **Ground vs Air Detection**: Different targeting behaviors for different contexts
- **Direct Line Trace**: Prioritizes what you're directly aiming at
- **Sphere Forgiveness**: Large radius fallback for easier TPS targeting
- **Debug Visualization**: Color-coded debug spheres show targeting behavior
- **Score-Based Selection**: Balances angle, distance, and vertical offset for best target

### Tempest Blade (Jump + Dash + Slash)
- **Status**: Planned for implementation
- **Difficulty**: High execution requirement
- **Effect**: Multi-hit aerial combo

---

## 💀 Ultimate System

### Mechanics
1. **Activation**: Abilities transform at 100% WP
2. **Grace Period**: 2-second window after reaching 100% WP to use abilities
3. **One-Time Use**: Using an ultimate permanently disables that ability
4. **Strategic Choice**: Choose which abilities to sacrifice
5. **Death Trigger**: 4th ultimate use or 3 abilities lost = death

### Ultimate Transformations
Each combat ability has an ultimate form with fully customizable stats:

#### **Firewall Breach** → Total System Compromise
- **Effect**: Instantly removes ALL armor from ALL enemies on screen
- **Editable Properties**:
  - Ultimate Radius Multiplier (1.0-5.0x base range)
  - Ultimate Armor Reduction (0.0-1.0, default 100%)
  - Ultimate Duration (1.0-30.0 seconds)

#### **System Override** → Total System Shutdown  
- **Effect**: Enhanced system override with longer duration and more damage
- **Editable Properties**:
  - Ultimate WP Cleanse Multiplier (1.0-3.0x)
  - Ultimate System Damage Multiplier (1.0-3.0x)
  - Ultimate System Duration Multiplier (1.0-3.0x)

#### **Pulse Hack** → EMP Blast
- **Effect**: Massive radius with full enemy stun instead of slow
- **Editable Properties**:
  - Ultimate Radius Multiplier (1.0-5.0x)
  - Ultimate Stun Duration (0.5-10.0 seconds)
  - Ultimate WP Cleanse (0.0-100.0)

#### **Data Spike** → System Corruption
- **Effect**: Pierces all enemies with enhanced data corruption
- **Editable Properties**:
  - Ultimate WP Cleanse (0.0-100.0)
  - Ultimate Projectile Damage Multiplier (1.0-5.0x)
  - Ultimate DOT Multiplier (1.0-10.0x)
  - Ultimate Pierce Count (0-20 enemies)

#### **Gravity Pull** → Singularity
- **Effect**: Creates black hole that pulls ALL enemies to central point
- **Editable Properties**:
  - Ultimate Singularity Distance (500.0-3000.0 units)
  - Ultimate Radius Multiplier (1.0-10.0x)
  - Ultimate Pull Force Multiplier (1.0-10.0x)

### Base Ultimate Properties
All abilities inherit these customizable properties:
- **Ultimate Range Multiplier** (1.0-10.0x)
- **Ultimate Damage Multiplier** (1.0-10.0x)  
- **Ultimate Duration Multiplier** (0.1-10.0x)

---

## 🏗️ Implementation Architecture

### Component Hierarchy
```cpp
UActorComponent
└── UAbilityComponent (Base class)
    ├── USlashAbilityComponent
    ├── UHackerDashAbility
    ├── UHackerJumpAbility
    ├── UFirewallBreachAbility
    ├── UPulseHackAbility
    ├── UGravityPullAbilityComponent
    ├── UDataSpikeAbility
    ├── USystemOverrideAbility
    └── UComboAbilityComponent (Base for combos)
        ├── UDashSlashCombo
        └── UJumpSlashCombo
```

### Key Systems
- **Resource Manager**: Handles WP (Stamina removed)
- **Combo Component**: Tracks input sequences
- **Combo System**: Executes combo patterns
- **Hit Stop Manager**: Provides combat impact feedback
- **Threshold Manager**: Handles WP thresholds, ultimate mode, and death

---

## 🎮 Blueprint Integration

### Ability Component Properties
All ability components expose these Blueprint-editable properties:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
float Damage = 20.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
float WPCost = 5.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
float Cooldown = 2.0f;
```

### Ultimate System Properties
All abilities inherit base ultimate properties:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Base")
float UltimateRangeMultiplier = 2.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Base")
float UltimateDamageMultiplier = 2.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Base")
float UltimateDurationMultiplier = 1.5f;
```

### Ability-Specific Ultimate Properties
Each ability has its own ultimate category for customization:

**Firewall Breach**:
```cpp
UPROPERTY(EditAnywhere, Category = "Ultimate Firewall Breach")
float UltimateRadiusMultiplier = 2.0f;

UPROPERTY(EditAnywhere, Category = "Ultimate Firewall Breach")
float UltimateArmorReduction = 1.0f;
```

**System Override**:
```cpp
UPROPERTY(EditAnywhere, Category = "Ultimate System Override")
float UltimateWPCleanseMultiplier = 1.5f;

UPROPERTY(EditAnywhere, Category = "Ultimate System Override")
float UltimateSystemDamageMultiplier = 1.5f;
```

### Combo-Specific Parameters
Combo components have additional parameters:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Timing")
float ComboWindowTime = 0.5f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Effects")
float TimeSlowScale = 0.3f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Targeting")
float AimForgivenessRadius = 180.0f;
```

### Workflow
1. Open player character Blueprint
2. Select ability components in Details panel
3. Find "Ultimate [AbilityName]" categories for ultimate customization
4. Adjust parameters without recompiling
5. Test changes immediately in PIE
6. Ultimate properties only take effect when used at 100% WP

---

## 📝 Notes

- All abilities are component-based for modularity
- Parameters are Blueprint-editable for rapid iteration
- Combo system uses individual components for each combo
- Focus is on Hacker path only (Forge path removed)
- UI systems have been removed for cleaner implementation
- Death system integrates with WP threshold mechanics

## 🔧 Recent Improvements (2025-07-11)

### Resource System Changes
- **Stamina Removed**: Entire stamina system removed from project
- **WP-Only System**: All abilities now use only WP or are free (basic abilities)
- **Interface Compatibility**: IResourceConsumer interface maintained for backward compatibility

### Dash Ability Fixes
- **Movement-Based Direction**: Dash now uses movement input direction instead of crosshair
- **W+Dash Special Case**: Forward dash follows camera angle for vertical movement
- **Fixed Input Mapping**: Corrected input axis interpretation

### Ultimate System Fixes
- **Fixed WP Instant Reset**: WP no longer resets immediately at 100%
- **Proper Ultimate Execution**: All abilities correctly execute ultimate versions at 100% WP
- **Ability Disabling**: Ultimate abilities properly disable after use and cannot be reused
- **Grace Period Removed**: No artificial delay - ultimates available immediately at 100% WP

### Targeting System Enhancements
- **Ground vs Air Targeting**: Combos now behave differently based on player state
- **Improved Aim Forgiveness**: Larger radius spheres for easier TPS targeting
- **Closest Enemy Priority**: When on ground, prioritizes closest enemy to prevent targeting past intended targets
- **Sophisticated Air Combat**: Multi-factor scoring system for fluid aerial combat

### Engine Editor Customization
- **Full Ultimate Customization**: All ultimate abilities now have editable properties in engine
- **Base Ultimate Properties**: Range, damage, and duration multipliers for all abilities
- **Ability-Specific Properties**: Each ability has unique ultimate parameters
- **Clear Organization**: Ultimate properties organized in dedicated categories

### Quality of Life
- **Debug Visualization**: Color-coded targeting visualization for combo development
- **Proper Inheritance**: Fixed property naming conflicts between base and derived classes
- **Documentation Updated**: Complete documentation of all new features and fixes