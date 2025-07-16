# Enemy AI Behaviors Documentation

## Overview
This document details the AI behavior patterns and combat mechanics for all enemy types in the Blackhole project.

## Enemy Types

### 1. Tank Enemy
**Role**: Defensive frontline unit
- **Health**: 150 HP
- **Behavior**: Defensive stance, area control
- **Special**: 50% damage reduction when blocking
- **Pattern**: Approach → Block → Ground slam → Retreat when low health

### 2. Combat Enemy
**Role**: Aggressive melee fighter
- **Health**: 75 HP
- **Behavior**: Direct assault, relentless pursuit
- **Special**: High mobility, no defensive options
- **Pattern**: Chase → Attack → Continue pressure

### 3. Hacker Enemy
**Role**: Support/Debuffer
- **Health**: 50 HP
- **Behavior**: Maintains distance, drains player resources
- **Special**: Mindmeld ability (1 WP/second drain)
- **Pattern**: Keep range → Mindmeld → Retreat if threatened

### 4. Agile Enemy (Assassin)
**Role**: Hit-and-run assassin
- **Health**: 75 HP
- **Behavior**: Assassin approach pattern
- **Special**: Backstab (2x damage + 1.5s stagger)
- **Pattern**: See detailed breakdown below

## Agile Enemy - Detailed Behavior

### Combat Pattern
The agile enemy follows a specific assassin pattern with four distinct phases:

#### Phase 1: Approaching
- Maintains 450-550 unit distance from player
- Circles and strafes to avoid attacks
- Waits for dash cooldown to end
- **Aggressive Pursuit**: Actively closes distance when dash is ready

#### Phase 2: Dash Attack (Assassin Approach)
- Triggers when:
  - Dash ability is ready AND player within 600 units
  - OR 5 seconds have passed (force attack timer)
- Dashes past the player to land behind them
- Executes backstab attack:
  - 2x damage multiplier (configurable)
  - Applies 1.5s stagger to player
  - Uses area damage (200 radius) for reliable hit detection

#### Phase 3: Retreating
- Immediately after successful backstab
- Retreats at 150% movement speed
- Maintains distance for exactly 3 seconds (configurable)
- Faces player while backing away

#### Phase 4: Maintaining
- Keeps distance between min/max range
- Performs evasive maneuvers
- May dodge if player aims at them
- Waits for dash cooldown before returning to Phase 1

### Configurable Parameters
All combat parameters are exposed in the editor:
- `MovementSpeed`: Base movement speed (default: 600)
- `DashCooldown`: Time between dashes (default: 2.0s)
- `BackstabDamageMultiplier`: Damage scaling (default: 2.0x)
- `BackstabStaggerDuration`: Player stun time (default: 1.5s)
- `MaintainDistanceMin`: Minimum keep-away distance (default: 450)
- `MaintainDistanceMax`: Maximum keep-away distance (default: 550)
- `RetreatDuration`: Time spent retreating (default: 3.0s)

### Special Behaviors
- **Force Attack Timer**: After 5 seconds of maintaining distance, forces an attack even if dash is on cooldown
- **Chase Behavior**: Uses custom `AgileChaseState` that prevents getting too close
- **Dodge Reactions**: 40% base chance to dodge, increased when player dashes

## State Machine Architecture

### Base States
All enemies use a state machine with these core states:
1. **Idle**: Default patrol/wait state
2. **Alert**: Player detected, preparing to engage
3. **Chase**: Actively pursuing player
4. **Combat**: In attack range, executing abilities
5. **Retreat**: Backing away (health/tactical)
6. **Dead**: Cleanup state

### State Transitions
- **Idle → Alert**: Player enters detection range (2500 units)
- **Alert → Chase**: Clear line of sight confirmed
- **Chase → Combat**: Within attack range
- **Combat → Retreat**: Low health or tactical withdrawal
- **Any → Dead**: Health reaches 0

### Custom State Implementations
- **AgileChaseState**: Maintains distance instead of closing
- **AgileCombatState**: Implements assassin pattern
- **TankCombatState**: Defensive stance and area attacks

## Combat Mechanics

### Damage System
- All damage routes through `TakeDamage()` function
- Applies to player's WP (Will Power) instead of health
- Supports damage multipliers and special effects

### Stagger System
- Disables target movement and input
- Slows animation playback (30% speed)
- Currently implemented for:
  - Player (via `ApplyStagger()`)
  - Enemies (via base class)

### Ability Components
Enemies use modular ability components:
- **SmashAbilityComponent**: Melee attacks (single/area)
- **DodgeComponent**: Evasive maneuvers
- **BlockComponent**: Damage reduction
- **MindmeldComponent**: WP drain effect

## AI Decision Making

### Target Prioritization
1. Current target persistence
2. Closest valid target
3. Most threatening target (future implementation)

### Ability Usage
- Cooldown-based system
- Random selection from available abilities
- Weighted by combat situation

### Movement Patterns
- **Direct Chase**: Straight line to target
- **Circle Strafe**: Orbital movement around target
- **Retreat**: Backing away while facing target
- **Flank**: Attempt to attack from sides (future)

## Performance Considerations

### Optimization Strategies
- State machines only tick when active
- Dead state disables all processing
- Efficient line-of-sight checks
- Pooled projectiles and effects

### Navigation
- Uses Unreal's navigation mesh
- Dynamic obstacle avoidance
- Path recalculation on interval

## Debugging

### Visual Indicators
- On-screen state display
- Distance measurements
- Cooldown timers
- Attack telegraphs

### Console Commands
- (Future) Enemy spawn commands
- (Future) AI behavior overrides
- (Future) State forcing

## Future Enhancements

### Planned Improvements
1. **Group Tactics**: Coordinated attacks
2. **Advanced Patterns**: More complex behaviors
3. **Adaptive AI**: Learn from player patterns
4. **Environmental Usage**: Use level geometry

### New Enemy Types (Planned)
- **Virus**: Area denial, spreading corruption
- **Firewall**: Super tank with shields
- **Script Kiddie**: Summons minions
- **Data Miner**: Steals player resources

---
*Last updated: 2025-07-16*