# Blackhole - Game Design Document
**Version**: 3.1  
**Engine**: Unreal Engine 5.5  
**Language**: C++ with Blueprint integration  
**Date**: 2025-07-08

> **📚 Note**: For detailed ability information, see [ABILITIES_DOCUMENTATION.md](ABILITIES_DOCUMENTATION.md)

## 🎮 Game Overview

### Core Concept
A tactical action game where players embody a hybrid hacker-forge warrior, choosing between two distinct paths before each level. Navigate dangerous environments through close combat and environmental manipulation while managing unique resource systems.

### Core Pillars
1. **Dual Path System**: Pre-level choice between Hacker (speed/precision) and Forge (power/destruction)
2. **Resource Risk/Reward**: Balance aggression with conservation through WP corruption or Heat buildup
3. **Dynamic Difficulty**: Ability loss creates escalating challenge with compensatory buffs
4. **Environmental Mastery**: Interact with world objects differently based on chosen path

## ⚡ Resource System

### Primary Resources
| Resource | Path | Start | Max | Mechanic |
|----------|------|-------|-----|----------|
| **Stamina** | Both | 100 | 100 | Universal resource, 10/sec regen |
| **Willpower** | Hacker | 0% | 100% | Corruption increases with ability use (bad) |
| **Heat** | Forge | 0 | 100 | Combat abilities only, 5/sec dissipation |

### Resource Interactions
- **Hacker**: Abilities cost Stamina + ADD WP corruption
- **Forge**: Combat abilities cost Stamina + Heat (utility uses stamina only)
- **Enemy Mindmeld**: Adds 1.0 WP/sec while maintaining line of sight
- **Combo Rewards**: Reduce WP by 2-10 or Heat by 3 based on chain length

## 🔀 Path System

### Hacker Path
- **Theme**: Speed, precision, technical mastery
- **Visual**: Cyan/blue effects
- **Playstyle**: Hit-and-run, environmental manipulation, ability combos
- **Risk**: WP corruption leads to ability loss

### Forge Path  
- **Theme**: Power, impact, destructive force
- **Visual**: Red/orange effects
- **Playstyle**: Tank through damage, area control, burst damage
- **Risk**: Heat overload causes temporary lockout

## ⚔️ Combat Abilities

### Hacker Abilities
| Ability | Key | Stamina | WP | Cooldown | Effect |
|---------|-----|---------|-----|----------|---------|
| **Katana Slash** | LMB | 10 | +15 | 2s | Quick strike with bleed DoT |
| **Firewall Breach** | RMB | 15 | +20 | 4s | 30% armor reduction for 5s |
| **Pulse Hack** | Q | 5 | +10 | 8s | AoE slow + cleanses 5 WP per enemy |
| **Gravity Pull** | E | 10 | +15 | 3s | Pull enemies/objects (2000 unit range) |
| **[Data Spike]** | R | - | - | - | *To be implemented* |
| **[System Override]** | F | - | - | - | *Ultimate - To be implemented* |

### Forge Abilities
| Ability | Key | Stamina | Heat | Cooldown | Effect |
|---------|-----|---------|------|----------|---------|
| **[Forge Slam]** | LMB | 15 | 20 | 2s | *Heavy ground strike - To be implemented* |
| **Molten Mace** | RMB | 20 | 30 | 5s | Cone attack with burn DoT |
| **Heat Shield** | Q | 15 | 20 | 12s | 100 HP absorption barrier |
| **Blast Charge** | E | 20 | 25 | 10s | Explosion with knockback |
| **Hammer Strike** | R | 15 | 20 | 6s | 3-hit stun combo |
| **[Volcanic Eruption]** | F | - | - | - | *Ultimate - To be implemented* |

### Utility Abilities
| Path | Dash (Shift) | Jump (Space) |
|------|--------------|--------------|
| **Hacker** | 5 Stamina, 1000 units, i-frames | 10 Stamina, double jump (0.5s cooldown between jumps) |
| **Forge** | 5 Stamina, damages on impact | 10 Stamina, ground slam on landing |

## 🎯 Ability Loss System

### WP System (Hacker Path)
| WP Range | Effect |
|----------|--------|
| 0-50% | Normal state - all abilities available |
| 50-99% | Buffed state - +20% damage, +15% cooldown reduction, +25% attack speed |
| 100% | **ULTIMATE MODE** - All abilities become ultimate versions |

**Ultimate Mode Mechanics**:
- When WP reaches 100%, non-basic abilities transform into powerful ultimate versions
- Basic abilities (Slash, Jump, Dash) remain normal and cannot be sacrificed
- Player can use ONE ultimate ability (from non-basic abilities)
- The ability used is permanently disabled for that combat
- WP resets to 0 after ultimate use
- Process can repeat until death conditions are met

**Death Conditions**:
- **Integrity reaches 0**: Standard combat death from damage
- **After losing 3 abilities**: If WP reaches 100% again → instant death
- **4th time reaching 100% WP**: Death (regardless of abilities lost)

### Ultimate Abilities (100% WP)
**Hacker Ultimate Abilities** (Non-Basic Only):
- **System Overload** (Pulse Hack): Screen-wide stun, cleanses 50 WP
- **Singularity** (Gravity Pull): Black hole pulls all enemies to center
- **Total System Compromise** (Firewall Breach): 100% armor removal on all enemies

**Basic Abilities** (Not Ultimate):
- **Slash**: Maintains normal function, only receives 50% WP buffs
- **Jump**: Maintains normal function, always available for mobility
- **Dash**: Maintains normal function, always available for repositioning

**Player Choice**: Use one ultimate ability, lose it permanently, reset to 0 WP

**Heat at 100% (Forge)**:
- *Meltdown*: AoE explosion dealing 50 damage to all nearby
- 5-second ability lockout (current implementation)
- Visible heat distortion effects
- Emergency vent animation

## 🌍 Environmental Interactions

### Hacker Interactions
| Object Type | Interaction | Effect |
|-------------|-------------|---------|
| **Data Terminals** | Hack (2s channel) | Reveal enemy positions, disable security |
| **Corrupted Nodes** | Cleanse (E ability) | -20 WP, creates safe zone |
| **Energy Barriers** | Bypass | Phase through for 3 seconds |
| **Turrets** | Override | Turn against enemies for 10s |

### Forge Interactions
| Object Type | Interaction | Effect |
|-------------|-------------|---------|
| **Metal Barriers** | Melt (3s channel) | Create new pathways |
| **Scrap Piles** | Forge | Craft temporary weapon upgrades |
| **Heat Vents** | Activate | -30 Heat instantly |
| **Destructibles** | Smash | Area damage + resource drops |

### Shared Interactions
- **Resource Stations**: Restore 25 Stamina
- **Combo Shrines**: Next combo grants double rewards
- **Path Beacons**: Preview path-specific routes

## 👾 Enemy Design

### Enemy Types
| Type | Health | Behavior | Special Ability |
|------|--------|----------|-----------------|
| **Hacker Scout** | 50 | Maintains distance | Mindmeld (1 WP/sec) |
| **Forge Brute** | 150 | Aggressive melee | Smash (40 damage + stun) |
| **Agile Assassin** | 75 | Hit and run | 30% dodge chance |
| **Tank Guardian** | 200 | Defensive | 50% damage reduction when blocking |
| **Hybrid Elite** | 100 | Adaptive AI | Switches tactics based on player path |

### AI Behaviors
- **Path Awareness**: Enemies adapt tactics to player's chosen path
- **Environmental Usage**: Use cover, activate traps, call reinforcements
- **Coordination**: Flanking maneuvers, combo attacks
- **Retreat Mechanics**: Low health enemies seek cover/healing

## ⚖️ Balance Framework

### Resource Economy
```
Stamina Costs (per ability tier):
- Basic: 5-10
- Standard: 10-20  
- Ultimate: 30-50

WP Corruption (per ability):
- Low impact: 5-10
- Medium impact: 10-20
- High impact: 20-30

Heat Generation:
- Light abilities: 10-20
- Heavy abilities: 20-30
- Ultimate: 40-50
```

### Damage Scaling
```
Base Damage:
- Light attacks: 10-20
- Medium attacks: 20-40
- Heavy attacks: 40-60
- DoT effects: 5-10/sec

Combo Multipliers:
- 3-chain: 1.25x
- 5-chain: 1.5x
- 7-chain: 2x
```

### Cooldown Guidelines
- Spam abilities: 1-2s
- Tactical abilities: 3-5s
- Strategic abilities: 8-12s
- Ultimates: 60-90s

## 🎨 Visual Design

### Path Identity
**Hacker Path**:
- Colors: Cyan, electric blue, white
- Effects: Digital glitches, data streams, holographic projections
- Audio: Electronic, synthetic, glitch sounds

**Forge Path**:
- Colors: Orange, red, molten yellow
- Effects: Heat distortion, sparks, molten metal
- Audio: Industrial, metal impacts, furnace sounds

### Environmental Themes
1. **Corrupted Datacenters**: Hacker-favored with hackable infrastructure
2. **Industrial Foundries**: Forge-favored with destructible elements
3. **Hybrid Zones**: Balanced opportunities for both paths

## 📈 Progression & Meta

### Skill Mastery
- **Combo Efficiency**: Maximize ability chains
- **Resource Management**: Optimal WP/Heat thresholds
- **Path Specialization**: Master unique mechanics
- **Environmental Exploitation**: Creative object usage

### Planned Features
1. **Ability Upgrades**: Enhance existing abilities with modifiers
2. **Path Synergies**: Unlock hybrid abilities for repeated playthroughs
3. **Environmental Mastery**: New interactions based on player skill
4. **Endless Mode**: Survival with escalating ability loss

## 🔧 Technical Implementation

### Architecture
- **Component-Based**: Modular ability system
- **Event-Driven**: Efficient delegate communication
- **No GAS**: Custom implementation for full control
- **Performance**: Object pooling, cached references

### Key Systems
| System | Purpose | Status |
|--------|---------|---------|
| ResourceManager | Central resource handling | ✅ Complete |
| ThresholdManager | Ability loss logic | ✅ Complete |
| ComboTracker | Chain detection | ✅ Complete |
| EnvironmentInteractor | Object interactions | 🔄 In Development |
| PathMechanics | 100% threshold events | 🔄 In Development |

### Console Commands
```
SetWP <0-100>          - Set Willpower corruption
SetHeat <0-100>        - Set Heat level
SetPath <Hacker|Forge> - Switch paths (dev only)
SetStamina <0-100>     - Set Stamina
StartCombat/EndCombat  - Toggle combat state
ShowDebugInfo          - Display all resources
SpawnEnemy <type>      - Spawn specific enemy
ForceUltimateMode      - Force activate ultimate mode
CacheAbilities         - Refresh ability tracking
ForceAbilityLoss <n>   - Disable n abilities for testing
```

## 📊 Metrics & KPIs

### Core Metrics
- **Average Combat Duration**: Target 60-90 seconds
- **Ability Usage Rate**: 1 ability per 3-5 seconds
- **WP/Heat Critical Events**: 1-2 per combat
- **Environmental Interactions**: 3-5 per level

### Balance Targets
- **Path Win Rate**: 48-52% for each path
- **Ability Usage Distribution**: No ability >30% of total
- **Resource Depletion**: Reach critical once per level
- **Death Distribution**: 40% resource management, 60% combat

## 🚀 Development Roadmap

### Phase 1: Core Polish (Current)
- ✅ All abilities implemented
- ✅ Resource system complete
- ✅ Basic enemy AI
- 🔄 VFX and audio integration

### Phase 2: Environmental Systems (Next)
- [ ] Hackable/Forgeable objects
- [ ] Path-specific routes
- [ ] Environmental hazards
- [ ] Interactive set pieces

### Phase 3: Advanced Features
- [ ] Missing abilities (R/F keys)
- [ ] 100% threshold mechanics
- [ ] Advanced enemy behaviors
- [ ] Boss encounters

### Phase 4: Content & Balance
- [ ] 5 unique levels
- [ ] Ability upgrades
- [ ] Endless mode
- [ ] Final balance pass

## 📝 Design Notes

### Critical Decisions
1. **No Path Switching**: Commitment creates meaningful choice
2. **Corruption vs Heat**: Different risk profiles for variety
3. **Ability Loss**: Creates dynamic difficulty within combat
4. **Environmental Focus**: Rewards exploration and creativity

### Open Questions
1. Should ultimate abilities have unique threshold interactions?
2. How do environmental interactions scale with player progression?
3. Should there be permanent upgrades between runs?
4. What happens when both paths are mastered - hybrid mode?

## 📝 Version History

### v3.1 (2025-07-08)
- Added death conditions: integrity=0, 3 abilities lost, 4th 100% WP
- Implemented Hacker jump cooldown (0.5s between jumps)
- Added automatic combat detection when enemies see player
- Consolidated all ability documentation into ABILITIES_DOCUMENTATION.md
- Added new console commands for testing ultimate system

### v3.0 (2025-07-07)
- Ultimate ability system implementation
- Basic abilities system (always available)
- Strategic ability sacrifice mechanic

---
*This document represents the complete game design. All systems marked ✅ are implemented and functional.*