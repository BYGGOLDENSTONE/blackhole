# Blackhole Project Status - Last Updated: 2025-01-02

## Project Overview
Unreal Engine 5.5 C++ third-person action game with component-based architecture. Features hack-based abilities, multiple enemy types, and dynamic camera switching.

## Core Systems Implemented

### 1. Component Architecture

#### Base Components
- **UAttributeComponent**: Base class for all attributes
  - Manages current/max values
  - Handles regeneration (configurable per attribute)
  - Provides percentage calculations for UI
  
- **UAbilityComponent**: Base class for all abilities
  - Manages cooldowns
  - Tracks resource costs
  - Defines ability range
  - Virtual Execute() function for implementation

#### Attribute Components
- **UIntegrityComponent** (Health)
  - No regeneration
  - TakeDamage() function
  - Respects blocking (50% damage reduction when blocking)
  
- **UStaminaComponent**
  - 10/sec regeneration rate
  - Used by physical abilities (Slash)
  
- **UWillPowerComponent**
  - No regeneration
  - Used by hack abilities (SystemFreeze, Kill)
  - Can be drained by Mindmeld

#### Ability Components
- **USlashAbilityComponent**: 20 damage, 10 stamina cost, 2s cooldown, 200 range
- **USystemFreezeAbilityComponent**: 10 will power, 2s stun, 3s cooldown, 3000 range (enemies only)
- **UKillAbilityComponent**: 40 will power, instant kill, 5s cooldown, 3000 range (enemies only)
- **USmashAbilityComponent**: 10 damage, 1.5s cooldown, 200 range
- **UBlockComponent**: Blocks physical damage (50% reduction), 2s cooldown
- **UDodgeComponent**: Quick movement dash, 2s cooldown
- **UMindmeldComponent**: 1 will/sec drain on sight, no cooldown

### 2. Character Classes

#### Player Character (ABlackholePlayerCharacter)
- **Components**: All 3 attributes + Slash, SystemFreeze, Kill abilities
- **Enhanced Input System** implemented with actions for:
  - Movement (WASD)
  - Looking (Mouse)
  - Jumping (Spacebar)
  - Abilities (LMB, 1, 2)
  - Camera Toggle (H)
- **Camera System**:
  - Third-person view (default)
  - First-person view (toggle with H)
  - Smooth transitions between views

#### Enemy Types
All enemies inherit from **ABaseEnemy** which provides:
- Integrity component
- "Enemy" tag for ability targeting
- Ragdoll physics on death
- Auto-cleanup after 10 seconds

**Enemy Variants:**
1. **ACombatEnemy**: Smash + Block + Dodge (balanced fighter)
2. **AAgileEnemy**: Smash + Dodge only (fast, can't block)
3. **ATankEnemy**: Smash + Block only (slow, high health, can't dodge)
4. **AHackerEnemy**: Mindmeld only (drains player's will power)

### 3. UI System (ABlackholeHUD)
- Player attribute bars (Integrity/Stamina/WillPower)
- Ability cooldown indicators
- Crosshair
- Target enemy information
- Color-coded displays

### 4. Game Mode (ABlackholeGameMode)
- Sets default player class
- Sets default HUD

## Key Features & Mechanics

### Combat System
- **Physical Abilities**: Use stamina, blocked by BlockComponent
- **Hack Abilities**: Use will power, only work on enemies
- **Line of Sight**: Required for ranged abilities
- **Cooldown Management**: Prevents ability spam

### Enemy AI Behaviors
- **Combat/Tank**: Chase and attack when in range
- **Agile**: Dodge when player is close
- **Hacker**: Maintain optimal distance for Mindmeld

### Death System
- Enemies ragdoll when Integrity reaches 0
- Physics impulse applied for dramatic effect
- Bodies cleaned up after 10 seconds
- All AI behavior stops on death

## Recent Fixes & Updates

1. **Fixed Mindmeld Not Working**:
   - Added `PrimaryComponentTick.bCanEverTick = true`
   - Fixed line of sight calculations
   - Added debug logging for troubleshooting

2. **Enhanced Input System**:
   - Replaced old input with Enhanced Input
   - Added camera toggle functionality
   - Prepared for runtime remapping

3. **Hack Ability Restrictions**:
   - SystemFreeze and Kill only work on enemies
   - Check for "Enemy" tag or ABaseEnemy inheritance

4. **Enemy Variants**:
   - Created AgileEnemy and TankEnemy
   - Each has unique ability combinations
   - Different stats and behaviors

## Blueprint Setup Required

### Input Actions to Create:
- `IA_Jump` (Digital)
- `IA_Move` (Axis2D)
- `IA_Look` (Axis2D)
- `IA_Slash` (Digital)
- `IA_SystemFreeze` (Digital)
- `IA_Kill` (Digital)
- `IA_ToggleCamera` (Digital)

### Input Mapping Context:
- Create `IMC_Default`
- Map all actions to appropriate keys

### Player Blueprint:
- Set all Input Action references
- Set DefaultMappingContext

### Enemy Setup:
- Ensure skeletal meshes have Physics Assets
- Configure collision properly for ragdoll

## Known Issues & Considerations

1. **Performance**: Multiple enemies using Mindmeld may impact performance due to tick updates
2. **Balance**: Will power has no regeneration - players can get permanently drained
3. **Visual Feedback**: Abilities need particle effects and animations
4. **Audio**: No sound effects implemented yet

## Next Steps Suggestions

1. **Polish**:
   - Add particle effects for abilities
   - Implement hit reactions and animations
   - Add sound effects

2. **Gameplay**:
   - Add will power regeneration pickups
   - Implement enemy spawning system
   - Create objectives/win conditions

3. **Technical**:
   - Implement save/load system
   - Add network replication for multiplayer
   - Optimize with object pooling

## File Structure
```
Source/Blackhole/
├── Components/
│   ├── Attributes/
│   │   ├── AttributeComponent.*
│   │   ├── IntegrityComponent.*
│   │   ├── StaminaComponent.*
│   │   └── WillPowerComponent.*
│   └── Abilities/
│       ├── AbilityComponent.*
│       ├── SlashAbilityComponent.*
│       ├── SystemFreezeAbilityComponent.*
│       ├── KillAbilityComponent.*
│       ├── SmashAbilityComponent.*
│       ├── BlockComponent.*
│       ├── DodgeComponent.*
│       └── MindmeldComponent.*
├── Player/
│   └── BlackholePlayerCharacter.*
├── Enemy/
│   ├── BaseEnemy.*
│   ├── CombatEnemy.*
│   ├── AgileEnemy.*
│   ├── TankEnemy.*
│   └── HackerEnemy.*
├── UI/
│   └── BlackholeHUD.*
└── Core/
    └── BlackholeGameMode.*
```