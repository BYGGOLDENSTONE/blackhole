# Blackhole - Game Design Document
**Version**: 4.2  
**Engine**: Unreal Engine 5.5  
**Language**: C++ with Blueprint integration  
**Date**: 2025-07-16

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
5. **Fluid Movement**: Wall running and advanced traversal enable cyberpunk mobility

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

### Critical State System
- **Entries**: Players have 3 critical state entries by default
- **Usage**: Each 0% WP consumes one entry
- **Timer Expires**: 
  - With entries: WP restored to 100%
  - No entries: Instant death
- **Future**: Collectibles can increase entry limit

## ⚔️ Combat System

### Basic Abilities (Always Available)
| Ability | Key | WP Cost | Effect |
|---------|-----|---------|--------|
| **Katana Slash** | LMB | 0 | Quick 3-hit combo, 20 base damage, dual detection (trace + sphere) |
| **Hacker Dash** | Shift | 0 | Movement input directional dash with i-frames |
| **Hacker Jump** | Space | 0 | Double jump with 0.5s cooldown |

### Advanced Movement System

#### Wall Running
The wall run system enables fluid cyberpunk traversal through vertical environments:

**Activation Requirements:**
- Player must be **airborne** (jumping, falling, or dashing)
- Wall must be within 45 units and roughly vertical (70-110° angle)
- Minimum wall height of 200 units
- Minimum height from ground: 150 units (prevents accidental wall runs)
- Player must be **looking at the wall** (40% dot product minimum) for intentional activation

**Mechanics:**
- **Speed Cap**: Dash speed (3000 units) capped at 1000 units/s during wall run
- **Height Maintenance**: Player runs at consistent height along wall surface  
- **Input Control**: W key required to continue wall running (any direction)
- **Duration**: Unlimited as long as W is held and wall exists
- **Camera Freedom**: Full 360° camera control while wall running for combat
- **Movement Lock**: Character follows wall path regardless of camera direction
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
- **StatusEffectComponent**: Centralized state management for all actors
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
| **Tank Unit** | 150 | Defensive + Charge | Heat aura (5 WP/sec) + Charge ability |
| **Agile Assassin** | 75 | Assassin approach | Backstab (2x damage + 1.5s stagger) |
| **Standard Soldier** | 100 | Basic melee | Builder component for psi-disruptor |
| **Mind Melder** | 75 | Long-range caster | 30s mindmeld (instant 0 WP) |

### AI Behaviors
- **Detection**: 2500 unit sight range
- **Combat**: Adaptive tactics based on player actions
- **Coordination**: Basic flanking and group tactics
- **Special**: Mindmeld maintains WP pressure

### Agile Assassin Details
- **Pattern**: Maintain 450-550 distance → Dash at 600 range → Backstab → 6s retreat
- **Abilities**:
  - **StabAttack**: Basic cone melee attack (15 damage, 150 range, 45° angle, 0.5s stagger)
  - **AssassinApproach**: Dash behind + backstab (2x damage, 1.5s stagger)
- **Aggressive**: Forces attack after 5 seconds if dash on cooldown
- **Configurable**: All combat parameters exposed in editor

### Tank Unit Details
- **Pattern**: Slow approach → Ground slam/charge based on distance
- **Abilities**:
  - **Heat Aura**: Passive 300 radius aura draining 5 WP/sec from all nearby
  - **Charge**: 300-1500 range dash dealing knockback damage
  - **Ground Slam**: AoE attack with knockback and stagger
- **Tanky**: 150 health, slow movement (300 units/s), heavy mass

### Standard Soldier Details
- **Pattern**: Basic chase → Sword attack → Coordinate building when alerted
- **Abilities**:
  - **Sword Attack**: Wide cone melee (20 damage, 180 range, 60° angle)
  - **Builder Component**: Can build psi-disruptor with 2+ soldiers
- **Psi-Disruptor**: 20s build time, disables dash/jump/wallrun in 2000 radius

### Mind Melder Details
- **Pattern**: Maintain 2000+ distance → Channel mindmeld → Retreat when damaged
- **Abilities**:
  - **Powerful Mindmeld**: 30s channel, drops player WP to 0 instantly
  - **Warning System**: Player notified of location and countdown
- **Fragile**: 75 health, interrupts on damage or close proximity (300 units)

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

## 🎯 Status Effect System

### StatusEffectComponent
A centralized system for managing all actor states:

**Supported Effects:**
- **Stagger**: Disables movement and actions, slows animations
- **Stun**: Complete incapacitation
- **Slow**: Reduces movement speed by magnitude
- **Freeze**: Prevents all movement
- **Knockdown**: Ground state with recovery
- **Invulnerable**: Damage immunity
- **SpeedBoost**: Increases movement speed
- **Dead**: Permanent death state

**Features:**
- Effect stacking with configurable limits
- Duration management with automatic cleanup
- Magnitude support for variable strength effects
- Event broadcasting for UI/VFX reactions
- Immunity system for special enemies/bosses

## 🔮 Upcoming Feature Implementations

### 1. Critical State Limit System
**Lives/Retry Mechanic**
- Players have limited "critical state entries" (default: 3 per level)
- Each time WP reaches 0%, one entry is consumed
- If timer expires without ultimate use:
  - Entries remaining: WP restored to 100% (full energy)
  - No entries left: Instant death
- When entries exceed limit, 0% WP = instant death (no timer)

**Progression Integration**
- Collectible items increase critical state limit
- Equipment upgrades grant additional entries
- Creates strategic resource management layer
- Encourages exploration and careful play

### 3. Slash Ability Targeting Enhancement
**Current Issue**: Trace from camera to crosshair often misses enemies positioned below/above crosshair

**New Implementation**:
1. **Extended Trace**: Camera ray extends 2x beyond crosshair distance
2. **Sphere Check**: 300-unit radius sphere around player for melee range
3. **Dual Validation**: Enemy must be hit by BOTH trace AND sphere
4. **Result**: More forgiving targeting while maintaining skill requirement

### 4. Enemy System Expansion
**Implemented Enemy Types** ✅:
- **Tank Unit**: Heat aura + charge ability
- **Standard Soldier**: Builder component for psi-disruptor
- **Mind Melder**: 30s instant-kill mindmeld
- **Agile Assassin**: Enhanced with 6s retreat and stab stagger

**Future Enemy Types** (Planned):
- **Virus**: Spreads corruption, area denial
- **Firewall**: Tank variant with shields
- **Script Kiddie**: Summons minions
- **Data Miner**: Resource stealing mechanics

**Enemy Ability Framework**:
- Modular ability components for enemies
- Data-driven ability assignment
- Cooldown and resource management
- Visual telegraphs for player reaction

## 🚀 Development Status

### Completed Features ✅
- Core combat system
- All Hacker abilities
- WP corruption system
- Ultimate/sacrifice mechanics
- Combo system (2 combos)
- Enemy AI system (8 enemy types)
- Death conditions
- Performance optimizations
- Data-driven architecture
- StatusEffectComponent system
- Psi-disruptor builder mechanics
- Enemy ability components

### In Development 🔄
- Additional combos
- Environmental interactions
- Advanced enemy types
- VFX/SFX polish
- UI implementation
- Wall run camera improvements
- Critical state limit system
- Slash ability targeting fix
- Additional enemy abilities

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

### v4.2 (2025-07-16) - Current
- Velocity indicator improvements (smaller, no direction, Y-150) → Removed completely
- Critical state WP restoration to 100% when entries remain
- Reverted momentum system per user feedback
- Wall run speed capped at 1000 when entering from dash
- Wall run height requirement implemented (150 units, trace from feet)
- Updated movement settings documentation

### v4.1 (2025-07-15)
- Complete WP energy system transformation
- Added critical state limit system (3 entries default)
- Implemented dual detection for slash ability
- Wall run minimum height requirement planned

### v4.0 (2025-07-11)
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

### v3.2 (2025-07-16)
- Wall run intentionality system (look requirement)
- Wall run free camera movement
- Agile enemy assassin behavior overhaul
- Player stagger system
- Enemy configurable combat stats
- StatusEffectComponent system for all actors
- Agile enemy ability components (StabAttack, AssassinApproach)

---

*This document reflects the current implementation. See linked documents for additional details.*