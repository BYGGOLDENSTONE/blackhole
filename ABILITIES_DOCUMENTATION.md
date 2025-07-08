# Blackhole - Complete Abilities Documentation
**Engine**: Unreal Engine 5.5  
**Last Updated**: 2025-07-08  
**Architecture**: Custom C++ Component System (No GAS)

## 📋 Table of Contents
1. [System Overview](#-system-overview)
2. [Abilities Quick Reference](#-abilities-quick-reference)
3. [Basic Abilities System](#-basic-abilities-system)
4. [Ultimate Abilities System](#-ultimate-abilities-system)
5. [Detailed Ability Descriptions](#-detailed-ability-descriptions)
6. [Implementation Architecture](#-implementation-architecture)
7. [Testing & Debug Commands](#-testing--debug-commands)

---

## 🎯 System Overview

### Core Concepts
- **Dual Resource System**: Stamina (universal) + Path-specific (WP/Heat)
- **Two Character Paths**: Hacker (WP corruption) and Forge (Heat management)
- **Basic Abilities**: Always available, cannot be disabled (Slash, Jump, Dash)
- **Ultimate System**: At 100% WP, abilities transform into powerful ultimates
- **Strategic Sacrifice**: Using an ultimate permanently disables that ability
- **Death Conditions**: 
  - Integrity reaches 0 (damage)
  - WP reaches 100% after losing 3 abilities
  - WP reaches 100% for the 4th time overall

### Resource Mechanics

#### Hacker Path - Willpower (WP)
- **0-49% WP**: Normal state
- **50-99% WP**: Buffed state (+20% damage, -15% cooldowns, +25% attack speed)
- **100% WP**: Ultimate mode activated
- **Corruption**: Using abilities ADDS WP (bad)
- **Cleansing**: Some abilities remove WP (good)

#### Forge Path - Heat
- **Normal**: 0-70% Heat
- **Warning**: 70-90% Heat
- **Overheat**: 90%+ Heat (abilities disabled for 3s)
- **Generation**: Abilities generate heat
- **Dissipation**: 5 Heat/second passive cooling

---

## ⚡ Abilities Quick Reference

| Ability | Key | Type | Target | Cooldown | Status |
|---------|-----|------|--------|----------|---------|
| **Katana Slash** | LMB | Melee | Front | 0.3s | ✅ (Basic) |
| **Forge Slam** | LMB | Melee AoE | Ground | 0.5s | 🔄 |
| **Kill (Debug)** | K | Instant Kill | Closest | 5s | ✅ |
| **Firewall Breach** | RMB | Debuff | Single | 4s | ✅ |
| **Molten Mace** | RMB | Melee | Front | 2s | ✅ |
| **Pulse Hack** | Q | AoE Slow | Area | 6s | ✅ |
| **Heat Shield** | Q | Shield | Self | 8s | ✅ |
| **Gravity Pull** | E | Pull | Target | 5s | ✅ |
| **Blast Charge** | E | Projectile | Line | 5s | ✅ |
| **Data Spike** | R | Projectile | Target | 4s | 🔄 |
| **Hammer Strike** | R | Melee Burst | Front | 7s | ✅ |
| **System Override** | F | Ultimate | AoE | 90s | 🔄 |
| **Volcanic Eruption** | F | Ultimate | AoE | 90s | 🔄 |
| **Hacker Dash** | Shift | Movement | Self | 1s | ✅ (Basic) |
| **Forge Dash** | Shift | Movement | Line | 2s | ✅ (Basic) |
| **Hacker Jump** | Space | Movement | Self | 0.5s* | ✅ (Basic) |
| **Forge Jump** | Space | Movement | AoE | 3s | ✅ (Basic) |

**Legend**:
- ✅ = Implemented
- 🔄 = In Development
- (Basic) = Always available, cannot be disabled/sacrificed
- *0.5s = Jump-to-jump cooldown (not ability cooldown)

---

## 🛡️ Basic Abilities System

Basic abilities are the foundation of combat - always available and never disabled.

### Design Philosophy
- **Consistency**: Players always have access to basic combat/movement
- **No Sacrifice**: Cannot be lost through ultimate system
- **No Ultimate Version**: Maintain normal function at 100% WP
- **Buff Eligible**: Still receive threshold buffs at 50% WP

### Basic Abilities List

#### Slash (LMB) - Universal Basic Attack
- **Katana Slash** (Hacker): Quick 3-hit combo
- **Forge Slam** (Forge): Heavy ground slam [In Development]
- Always available for basic damage output
- Receives damage buffs from WP thresholds

#### Jump (Space) - Vertical Mobility
- **Hacker Jump**: Double jump with air control
  - 0.5s cooldown between jumps
  - Enhanced UI shows jump cooldown
- **Forge Jump**: Single powerful jump with slam damage
- Essential for platforming and dodging

#### Dash (Shift) - Horizontal Mobility
- **Hacker Dash**: Phase dash with brief invulnerability
- **Forge Dash**: Charge dash with knockback
- Core repositioning tool

---

## ⚡ Ultimate Abilities System

### System Activation
When Hacker path WP reaches 100%:
1. Non-basic abilities transform into ultimate versions
2. Player chooses ONE ultimate to use
3. Used ability is permanently disabled
4. WP resets to 0
5. Process repeats until death conditions

### Death Conditions
- **After 3 abilities lost**: Next 100% WP = instant death
- **4th time at 100% WP**: Death (regardless of abilities lost)
- **Integrity at 0**: Standard combat death

### Ultimate Transformations

#### Pulse Hack → "System Overload"
- **Normal**: 500 radius slow, 3s duration
- **Ultimate**: 
  - 2000 unit radius (screen-wide)
  - Full 3s stun (not just slow)
  - Cleanses 50 WP instantly
  - Affects ALL enemies on screen

#### Gravity Pull → "Singularity"
- **Normal**: Pull single target 2000 units
- **Ultimate**:
  - 6000 unit radius black hole
  - Pulls ALL enemies and physics objects
  - 5x pull force
  - Creates singularity point ahead of player

#### Firewall Breach → "Total System Compromise"
- **Normal**: -30% armor on single target
- **Ultimate**:
  - 3000 unit radius
  - 100% armor removal (full damage)
  - 10s duration
  - Affects ALL enemies in range

### Strategy Considerations
- **Choose Wisely**: Each ultimate serves different tactical needs
- **Plan Your Build**: Consider which abilities to keep
- **Risk vs Reward**: Push for ultimates or play safe?

---

## 📖 Detailed Ability Descriptions

### Hacker Path Abilities

#### Firewall Breach (RMB) ✅
**Type**: Single Target Debuff  
**Resources**: 15 Stamina, +20 WP  
**Cooldown**: 4 seconds  
**Range**: 300 units  

**Mechanics**:
- Instant cast armor reduction
- Target takes 30% more damage for 5s
- Visible corruption effect
- Can stack with other debuffs

**Ultimate - Total System Compromise**:
- ALL enemies in 3000 units
- 100% armor removal
- 10 second duration
- Massive tactical advantage

#### Pulse Hack (Q) ✅
**Type**: AoE Crowd Control  
**Resources**: 20 Stamina, +25 WP  
**Cooldown**: 6 seconds  
**Radius**: 500 units  

**Mechanics**:
- 50% movement slow for 3s
- Cleanses 5 WP per affected enemy
- Blue pulse visual
- Enemies show hack effect

**Ultimate - System Overload**:
- Screen-wide effect (2000 units)
- Full stun instead of slow
- Instant 50 WP cleanse
- Game-changing crowd control

#### Gravity Pull (E) ✅
**Type**: Single Target Displacement  
**Resources**: 18 Stamina, +18 WP  
**Cooldown**: 5 seconds  
**Range**: 2000 units  

**Mechanics**:
- Pulls target to player
- 0.5s channel time
- Can grab aerial enemies
- Interrupts enemy attacks

**Ultimate - Singularity**:
- Creates gravity well
- Pulls ALL enemies in 6000 units
- Affects physics objects
- Sets up massive combos

#### Hacker Dash (Shift) ✅ [BASIC]
**Type**: Phase Movement  
**Resources**: 10 Stamina  
**Cooldown**: 1 second  
**Distance**: 1000 units  

**Mechanics**:
- 0.3s invulnerability frames
- Pass through enemies
- Leaves digital trail
- Can dash in air

#### Hacker Jump (Space) ✅ [BASIC]
**Type**: Enhanced Vertical Movement  
**Resources**: 10 Stamina  
**Cooldown**: 0.5s between jumps  
**Height**: 600 units  

**Mechanics**:
- Double jump capability
- Enhanced air control
- No fall damage
- UI shows jump cooldown

### Forge Path Abilities

#### Molten Mace Slash (RMB) ✅
**Type**: Heavy Melee  
**Resources**: 12 Stamina, +18 Heat  
**Cooldown**: 2 seconds  
**Range**: 400 units  

**Mechanics**:
- Wide horizontal sweep
- Applies burn DoT (30 damage/3s)
- Knocks back enemies
- Leaves fire trail

#### Heat Shield (Q) ✅
**Type**: Defensive Buff  
**Resources**: 15 Stamina  
**Cooldown**: 8 seconds  
**Duration**: 5 seconds  

**Mechanics**:
- 50% damage reduction
- Removes 20 Heat on activation
- Reflects 20% damage as fire
- Orange shield visual

#### Blast Charge (E) ✅
**Type**: Explosive Projectile  
**Resources**: 20 Stamina, +25 Heat  
**Cooldown**: 5 seconds  
**Range**: 1500 units  

**Mechanics**:
- Travels in straight line
- Explodes on impact (300 radius)
- 150 direct + 75 splash damage
- Applies burn to all hit

#### Hammer Strike (R) ✅
**Type**: Devastating Melee  
**Resources**: 25 Stamina, +30 Heat  
**Cooldown**: 7 seconds  
**Range**: 500 units  

**Mechanics**:
- 360° spin attack
- Massive 300 damage
- Stuns for 1 second
- Creates shockwave effect

#### Forge Dash (Shift) ✅ [BASIC]
**Type**: Charge Movement  
**Resources**: 10 Stamina  
**Cooldown**: 2 seconds  
**Distance**: 800 units  

**Mechanics**:
- Damages enemies in path
- Knocks enemies aside
- Faster but shorter than Hacker
- Builds momentum

#### Forge Jump (Space) ✅ [BASIC]
**Type**: Power Slam Jump  
**Resources**: 15 Stamina  
**Cooldown**: 3 seconds  
**Height**: 800 units  

**Mechanics**:
- Single powerful jump
- Slam damage on landing
- Creates shockwave
- Area damage on impact

### Debug Ability

#### Kill (K) ✅
**Type**: Instant Elimination  
**Resources**: None  
**Cooldown**: 5 seconds  
**Range**: Auto-target closest  

**Purpose**: Testing tool for development
- Instantly kills nearest enemy
- Ignores all defenses
- Not balanced for gameplay
- Shows debug particles

---

## 🏗️ Implementation Architecture

### Component Hierarchy
```
UAbilityComponent (Base)
├── UBasicAbility
│   ├── USlashAbilityComponent
│   └── UKillAbilityComponent
├── UUtilityAbility  
│   ├── UHackerDashAbility
│   ├── UForgeDashAbility
│   ├── UHackerJumpAbility
│   └── UForgeJumpAbility
├── UHackerAbility
│   ├── UFirewallBreachAbility
│   ├── UPulseHackAbility
│   └── UGravityPullAbilityComponent
└── UForgeAbility
    ├── UMoltenMaceSlashAbility
    ├── UHeatShieldAbility
    ├── UBlastChargeAbility
    └── UHammerStrikeAbility
```

### Key Systems

#### AbilityComponent Base
```cpp
// Core properties
bool bIsBasicAbility;      // Cannot be disabled
bool bIsInUltimateMode;    // Ultimate version active
float CurrentCooldown;     // Cooldown tracking
float StaminaCost;         // Universal resource
float WPCost;              // Hacker corruption
float HeatCost;            // Forge heat
```

#### ThresholdManager
- Tracks WP thresholds and combat state
- Manages ultimate mode activation
- Handles ability disabling
- Triggers death conditions
- Provides survivor buffs

#### ResourceManager
- Manages Stamina/WP/Heat
- Broadcasts resource changes
- Handles heat dissipation
- Tracks WP threshold states

### Data Flow
```
Input → AbilityComponent → CanExecute() → Resource Check
                ↓                              ↓
         Execute/ExecuteUltimate ← Ultimate Mode Check
                ↓
         ThresholdManager ← Notify if Ultimate Used
                ↓
         Disable Ability & Reset WP
```

---

## 🐛 Testing & Debug Commands

### Console Commands
```cpp
// Resource manipulation
SetWP <0-100>              // Set WP percentage
SetHeat <0-100>            // Set Heat percentage  
SetStamina <0-100>         // Set Stamina percentage

// Combat testing
ForceUltimateMode          // Activate ultimate mode
ForceAbilityLoss <count>   // Disable abilities
CacheAbilities             // Refresh ability tracking
StartCombat                // Start combat manually
EndCombat                  // End combat state

// Debug display
ShowDebugInfo              // Show ability states
ToggleResourceDebug        // Show resource flow
```

### Testing Scenarios

#### Ultimate System Test
```
1. SetWP 99
2. Use any Hacker ability to reach 100%
3. Verify ultimate modes activate
4. Use an ultimate ability
5. Verify ability disabled and WP reset
```

#### Death Trigger Test
```
1. ForceAbilityLoss 3
2. SetWP 100
3. Verify instant death
```

#### Combat Flow Test
```
1. Approach enemies (auto-combat start)
2. Use abilities to build WP
3. Trigger ultimate at 100%
4. Continue until death conditions
```

### Debug Visualizations
- Purple lines: Mindmeld connections
- Blue circles: Ability ranges
- Orange bars: Heat levels
- Cyan text: Buffed abilities
- Yellow text: Ultimate abilities
- Red X: Disabled abilities

---

## 📝 Development Notes

### Adding New Abilities
1. Create component inheriting from appropriate base
2. Override `Execute()` and optionally `ExecuteUltimate()`
3. Set resource costs in constructor
4. Mark basic abilities with `bIsBasicAbility = true`
5. Add to player character blueprint
6. Update input mappings

### Balance Guidelines
- Basic abilities: Low cost, short cooldown
- Normal abilities: Moderate cost, tactical use
- Ultimate abilities: No cost, massive impact
- Death prevention: Max 3 ability losses

### Known Issues
- Jump cooldown UI only shows between jumps
- Ultimate VFX need enhancement
- Some abilities missing (Data Spike, ultimates)

---

*This document consolidates all ability-related information. For general project status, see PROJECT_STATUS.md*