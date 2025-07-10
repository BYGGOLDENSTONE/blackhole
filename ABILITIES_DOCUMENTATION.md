# Blackhole - Hacker Path Abilities Documentation
**Engine**: Unreal Engine 5.5  
**Last Updated**: 2025-07-10  
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
- **Dual Resource System**: Stamina (universal) + Willpower (WP)
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
- **Type**: Quick teleport dash
- **Distance**: 500 units
- **Stamina Cost**: 10
- **Cooldown**: 1 second
- **Special Features**:
  - Phase through enemies
  - Brief invulnerability frames
  - Can combo with Slash for "Phantom Strike"

### Hacker Jump (Space)
- **Type**: Enhanced double jump
- **Jump Height**: Variable based on hold time
- **Stamina Cost**: 5 per jump
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

Combo abilities are executed by specific input sequences and use the component-based system.

### Phantom Strike (Dash + Slash)
- **Component**: `UDashSlashCombo`
- **Input Window**: 0.5 seconds
- **Effect**: Teleport behind enemy for backstab
- **Damage**: 150% slash damage × 2.0 backstab multiplier
- **Blueprint Parameters**:
  - Teleport Distance (150 units)
  - Backstab Multiplier (2.0x)
  - Time Slow Scale (0.1)
  - Auto-rotate to target (true)

### Aerial Rave (Jump + Slash)
- **Component**: `UJumpSlashCombo`
- **Input Window**: 0.3 seconds while airborne
- **Effect**: Downward slash with shockwave
- **Damage**: 125% slash + 50 shockwave damage
- **Blueprint Parameters**:
  - Shockwave Radius (300 units)
  - Shockwave Damage (50)
  - Downward Force (1000)
  - Damage falloff with distance

### Tempest Blade (Jump + Dash + Slash)
- **Status**: Planned for implementation
- **Difficulty**: High execution requirement
- **Effect**: Multi-hit aerial combo

---

## 💀 Ultimate System

### Mechanics
1. **Activation**: Abilities transform at 100% WP
2. **One-Time Use**: Using an ultimate permanently disables that ability
3. **Strategic Choice**: Choose which abilities to sacrifice
4. **Death Trigger**: 4th ultimate use or 3 abilities lost = death

### Ultimate Transformations
Each combat ability has an ultimate form:
- **Firewall Breach** → Mass system corruption
- **Pulse Hack** → EMP blast
- **Gravity Pull** → Black hole singularity
- **Data Spike** → Reality tear
- **System Override** → Total digital dominance

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
- **Resource Manager**: Handles Stamina and WP
- **Combo Component**: Tracks input sequences
- **Combo System**: Executes combo patterns
- **Hit Stop Manager**: Provides combat impact feedback
- **Threshold Manager**: Handles WP thresholds and death

---

## 🎮 Blueprint Integration

### Ability Component Properties
All ability components expose these Blueprint-editable properties:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
float Damage = 20.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
float StaminaCost = 10.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
float WPCost = 5.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
float Cooldown = 2.0f;
```

### Combo-Specific Parameters
Combo components have additional parameters:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Timing")
float ComboWindowTime = 0.5f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Effects")
float TimeSlowScale = 0.3f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Damage")
float DamageMultiplier = 1.5f;
```

### Workflow
1. Open player character Blueprint
2. Select ability components in Details panel
3. Adjust parameters without recompiling
4. Test changes immediately in PIE

---

## 📝 Notes

- All abilities are component-based for modularity
- Parameters are Blueprint-editable for rapid iteration
- Combo system uses individual components for each combo
- Focus is on Hacker path only (Forge path removed)
- UI systems have been removed for cleaner implementation
- Death system integrates with WP threshold mechanics