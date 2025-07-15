# Blackhole - Game Design Document
**Version**: 4.1  
**Engine**: Unreal Engine 5.5  
**Language**: C++ with Blueprint integration  
**Date**: 2025-07-15

> **📚 Note**: For detailed ability information, see [ABILITIES_DOCUMENTATION.md](ABILITIES_DOCUMENTATION.md)
> 
> **🔧 Note**: For future Forge path implementation, see [FORGE_PATH_FUTURE_IMPLEMENTATION.md](FORGE_PATH_FUTURE_IMPLEMENTATION.md)
>
> **📊 Note**: For code quality audit results, see [AUDIT_REPORT.md](AUDIT_REPORT.md)

## 🎮 Game Overview

### Core Concept
A high-intensity action game where players embody a digital warrior navigating cyberspace through precise combat and ability management. Master the corruption system where power comes at the cost of stability.

### Core Pillars
1. **Risk/Reward Combat**: Balance aggressive ability use with corruption buildup
2. **Dynamic Difficulty**: Ability loss creates escalating challenge with compensatory buffs
3. **Precision Gameplay**: Combo system rewards timing and skill
4. **Strategic Sacrifice**: Choose which abilities to lose when pushed to the limit
5. **Fluid Movement**: Wall running and momentum preservation enable cyberpunk traversal

## ⚡ Resource System

### Energy System Design
**REVOLUTIONARY CHANGE**: WP is now an energy/mana system with unique ultimate mechanics

| Resource | Start | Max | Mechanic |
|----------|-------|-----|----------|
| **Willpower (WP)** | 100 | 100 | Energy for abilities and health |
| **Stamina** | - | - | Removed from the game |

### WP Mechanics
- **Taking Damage**: Reduces WP (drains energy)
- **Using Abilities**: Consumes WP (energy cost)
- **Killing Enemies**: Restores WP based on enemy type
- **Combos**: Restore WP (Dash+Slash: +15, Jump+Slash: +15, Dash+WallRun: +10)
- **At 0 WP**: Ultimate mode activates (no death)
- **Ultimate Reset**: Using ultimate ability resets WP to 100 (full energy)

### Resource Interactions
- **Abilities**: Consume WP as energy cost
- **Basic Abilities**: Always available (still cost WP)
- **Enemy Mindmeld**: Drains 1.0 WP/sec while maintaining line of sight
- **Combo Rewards**: Restore WP on successful execution

## 🎯 Hacker Path Mechanics

### Theme & Identity
- **Core**: Speed, precision, technical mastery
- **Visual**: Cyan/blue digital effects, glitch aesthetics
- **Audio**: Electronic, synthetic, cyberpunk soundscape
- **Playstyle**: Hit-and-run tactics, ability combos, environmental manipulation

### Willpower (WP) Thresholds
| WP Range | State | Effects |
|----------|-------|---------|
| 0% | Ultimate Mode | Critical timer (5s) + All abilities become ultimates |
| 1-20% | Critical Low | Low energy warning |
| 20-50% | Warning | Reduced buffs |
| 50-100% | Healthy | +20% damage, -15% cooldowns, +25% attack speed |

## ⚔️ Combat System

### Basic Abilities (Always Available)
| Ability | Key | WP Cost | Effect |
|---------|-----|---------|--------|
| **Katana Slash** | LMB | 0 | Quick 3-hit combo, 20 base damage |
| **Hacker Dash** | Shift | 0 | Movement input directional dash with i-frames |
| **Hacker Jump** | Space | 0 | Double jump with 0.5s cooldown |

### Advanced Movement System

#### Wall Running
The wall run system enables fluid cyberpunk traversal through vertical environments:

**Activation Requirements:**
- Player must be **airborne** (jumping, falling, or dashing)
- Wall must be within 45 units and roughly vertical (70-110° angle)
- Minimum wall height of 200 units

**Mechanics:**
- **Speed Preservation**: Dash momentum (3000 units) is fully preserved during wall run
- **Height Maintenance**: Player runs at consistent height along wall surface  
- **Input Control**: W key required to continue wall running
- **Duration**: Unlimited as long as W is held and wall exists
- **Camera Effects**: Dynamic camera tilt based on wall side (±20°)

**Wall Jump System:**
- **Activation**: SPACE key during wall run triggers diagonal wall jump
- **Direction**: Launches diagonally away from wall (up + left/right)
- **Launch Force**: 550 units with reliable physics using Launch() method
- **Cooldown**: 1.5s cooldown prevents immediate wall re-attachment

**Safety Systems:**
- **Wall Verification**: Automatic detection every second prevents "air running"
- **Ability Restrictions**: Blocks dash and regular jump during wall run
- **Advanced Abilities**: Combat abilities remain available during wall run
- **Auto-End**: Wall run ends automatically when wall geometry ends

**Integration:**
- Works seamlessly with existing dash and jump abilities
- Preserves momentum for chaining movement techniques
- Visual feedback through UI messages and particle effects
- Debug visualization available for level designers

### Combat Abilities
| Ability | Key | WP Cost | Cooldown | Effect |
|---------|-----|---------|----------|---------|
| **Firewall Breach** | RMB | 15 | 4s | Mark enemy for +30% damage |
| **Pulse Hack** | Q | 20 | 6s | AoE slow (50%) for 3s |
| **Gravity Pull** | E | 15 | 5s | Pull enemies (1000 range) |
| **Data Spike** | R | 25 | 4s | High damage projectile |
| **System Override** | F | 30 | 90s | AoE stun |

### Combo System
| Combo | Input | Window | Effect |
|-------|-------|--------|--------|
| **Phantom Strike** | Dash → Slash | 0.5s | Teleport backstab (2x damage) |
| **Aerial Rave** | Jump → Slash | 0.3s | Ground slam with shockwave |

> **Note**: Additional combos (Tempest Blade, Blade Dance) planned for future updates

## 💀 Ultimate System

### Ultimate Mode (0% WP)
When WP reaches 0%, the player enters Ultimate Mode:
1. **Critical Timer Starts**: 5 seconds to act
2. All non-basic abilities become ultimate versions
3. Player must use ONE ultimate ability before timer expires
4. The used ability is permanently disabled
5. WP resets to 100% after use (full energy)
6. If timer expires: Death
7. Process repeats - each energy depletion trades power for sacrifice

### Death Conditions
1. **All Abilities Disabled**: Death when no abilities remain (implementation pending)
2. **Enemy Pressure**: Mindmeld continuously drains WP

### Strategic Choices
- Which abilities to sacrifice?
- When to use ultimates?
- Risk immediate power vs long-term survival

## 🏗️ Technical Architecture

### Core Systems (Post-Audit)

#### Component Architecture
- **Base**: Component-based ability system
- **Interfaces**: `IResourceConsumer` for clean abstraction
- **Subsystems**: Dedicated managers for specific responsibilities
- **Performance**: 38% improvement through optimization

#### Key Systems
| System | Purpose | Score |
|--------|---------|-------|
| **ResourceManager** | Stamina/WP management | 8/10 |
| **BuffManager** | Combat buff stacking | 9/10 |
| **DeathManager** | Death condition tracking | 8.8/10 |
| **ComboDetectionSubsystem** | Input-based combos | 8.8/10 |
| **ObjectPoolSubsystem** | Performance optimization | 9/10 |
| **AbilityComponent** | Base ability framework | 8/10 |
| **WallRunComponent** | Wall running and traversal | 9/10 |

#### Data-Driven Design
- **ComboDataAsset**: Designer-friendly combo creation
- **Blueprint Parameters**: All ability values exposed
- **Hot Reload**: Modify values without recompiling

### Console Commands
```
# Resource Management
SetWP <0-100>          - Set Willpower percentage
SetStamina <0-100>     - Set Stamina
SetIntegrity <0-100>   - Set Health

# Combat Testing
StartCombat/EndCombat  - Toggle combat state
ForceUltimateMode      - Activate ultimate mode
ForceAbilityLoss <n>   - Disable n abilities

# Debug
ShowDebugInfo          - Display all resources
SpawnEnemy <type>      - Spawn specific enemy
TestCombo <name>       - Test specific combo
```

## 👾 Enemy System

### Current Enemy Types
| Type | Health | Behavior | Special |
|------|--------|----------|---------|
| **Hacker Scout** | 50 | Ranged attacks | Mindmeld (1 WP/sec) |
| **Combat Drone** | 75 | Aggressive melee | High mobility |
| **Tank Unit** | 150 | Defensive | 50% damage reduction |
| **Agile Assassin** | 75 | Hit and run | 30% dodge chance |

### AI Behaviors
- **Detection**: 2500 unit sight range
- **Combat**: Adaptive tactics based on player actions
- **Coordination**: Basic flanking and group tactics
- **Special**: Mindmeld maintains WP pressure

## 🎨 Visual & Audio Design

### Visual Identity
- **Color Palette**: Cyan, electric blue, dark backgrounds
- **Effects**: Digital glitches, data streams, holographic trails
- **Combat**: Time slow effects, particle trails, impact feedback
- **UI**: Minimalist cyberpunk aesthetic

### Audio Design
- **Combat**: Electronic impacts, digital distortion
- **Abilities**: Unique sound signatures per ability
- **Feedback**: Clear audio cues for combos and ultimates
- **Ambience**: Cyberpunk atmosphere

## 📈 Balance & Progression

### Resource Economy
```
Stamina Costs:
- Movement: 5-10
- Combat: 10-20
- Ultimate: 30+

WP Generation:
- Basic attacks: +2
- Light abilities: +10-15
- Heavy abilities: +20-30
- Ultimate: +30-40
```

### Damage Scaling
```
Base Values:
- Light attacks: 20
- Abilities: 30-80
- Combos: 1.5-2x multiplier
- Ultimates: 2-3x base

Buff Scaling:
- 50%+ WP: +20% damage
- Per ability lost: +10% damage
```

## 🚀 Development Status

### Completed Features ✅
- Core combat system
- All Hacker abilities
- WP corruption system
- Ultimate/sacrifice mechanics
- Combo system (2 combos)
- Enemy AI basics
- Death conditions
- Performance optimizations
- Data-driven architecture

### In Development 🔄
- Additional combos
- Environmental interactions
- Advanced enemy types
- VFX/SFX polish
- UI implementation

### Future Plans 📋
- Forge path (see separate doc)
- Level design
- Boss encounters
- Progression system
- Multiplayer considerations

## 📊 Performance Metrics

### Current Performance (Post-Audit)
- **Ability Tick Time**: 1.3ms (-38%)
- **Memory Usage**: 90MB (-25%)
- **Object Pooling**: Active for projectiles
- **Blueprint Compatibility**: 100% maintained

### Optimization Features
- Selective ability ticking
- Object pooling for effects
- Efficient resource checking
- Cached component references

## 🎯 Design Philosophy

### Core Principles
1. **Risk Creates Reward**: Higher WP enables stronger abilities
2. **Choice Matters**: Permanent consequences for ultimate use
3. **Skill Expression**: Combos and timing windows
4. **Clear Feedback**: Visual/audio clarity for all actions

### Player Experience Goals
- **Tension**: Constant WP management decisions
- **Power Fantasy**: Ultimate abilities feel impactful
- **Mastery**: Skill ceiling through combos and resource management
- **Variety**: Multiple viable playstyles

## 📝 Version History

### v4.0 (2025-07-11) - Current
- Separated Forge path to future implementation doc
- Added architectural improvements from code audit
- Updated with current implementation status
- Added performance metrics
- Focused on Hacker-only gameplay

### v3.2 (2025-07-09)
- Complete combo system implementation
- Added timing windows and discounts
- Integrated combo detection

### v3.1 (2025-07-08)
- Death condition system
- Ultimate ability framework
- Combat state detection

---

*This document reflects the current implementation. See linked documents for additional details.*