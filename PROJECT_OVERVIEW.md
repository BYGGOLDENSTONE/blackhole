# Blackhole - UE5 Component-Based Action Game

## Project Summary
A third-person action game built in Unreal Engine 5.5 using a modular component-based architecture. Players use hack-based abilities to defeat various enemy types while managing stamina and willpower resources.

## Core Architecture

### Component System
The game uses two primary component types:
- **Attribute Components**: Health (Integrity), Stamina, and WillPower
- **Ability Components**: Various combat and utility abilities with cooldowns

### Character Classes
- **Player (BlackholePlayerCharacter)**: All attributes + Slash, SystemFreeze, Kill abilities
- **Enemy Types**:
  - **CombatEnemy**: Balanced fighter with Smash, Block, Dodge
  - **AgileEnemy**: Fast attacker with Smash and Dodge only
  - **TankEnemy**: Heavy defender with Smash and Block only
  - **HackerEnemy**: Drains player's willpower with Mindmeld

## Key Features

### Combat System
- **Physical attacks** (Slash, Smash) consume stamina and can be blocked
- **Hack abilities** (SystemFreeze, Kill) consume willpower and only target enemies
- **Defensive abilities** (Block reduces damage 50%, Dodge provides quick movement)
- **Mindmeld** drains willpower on line-of-sight

### Camera System
- Toggle between first/third person with H key
- Body remains visible in first person (head hidden)
- Camera-based aiming for accurate targeting

### Visual Systems
- Weapon/shield attachment via skeletal mesh sockets
- Dynamic shield visibility during blocking
- Ragdoll physics on death with auto-cleanup

## Technical Implementation

### Input System
Using UE5's Enhanced Input with mappings for:
- Movement (WASD), Look (Mouse), Jump (Space)
- Abilities: Slash (LMB), SystemFreeze (1), Kill (2)
- Camera Toggle (H)

### Socket Configuration
- **camerasocket**: Head bone for first-person camera
- **weaponsocket**: Right hand for weapons
- **shieldsocket**: Left forearm for shields

### UI System
- Attribute bars with color coding
- Cooldown indicators
- Crosshair and target information

## Setup Instructions

### Code Setup
1. Place source files in project's Source folder
2. Regenerate project files
3. Compile in Unreal Editor
4. Set BlackholeGameMode as default

### Blueprint Configuration
1. Create Input Actions and Mapping Context
2. Create player/enemy blueprints from C++ classes
3. Configure sockets on skeletal mesh
4. Assign meshes to weapon/shield components
5. Set HeadBoneName property (e.g., "head", "neck_01")

### Testing Checklist
- [ ] First/third person camera toggle works
- [ ] Body visible in first person, head hidden
- [ ] Weapons/shields appear correctly
- [ ] All abilities function with proper cooldowns
- [ ] Enemies ragdoll on death
- [ ] Mindmeld stops when enemy dies

## Recent Updates (January 2025)
- Fixed Mindmeld continuing after enemy death
- Improved first-person camera with body visibility
- Added camera-based aiming for player abilities
- Implemented socket-based equipment system
- Removed debug lines from player abilities

## Known Limitations
- No willpower regeneration (players can be permanently drained)
- Missing visual effects and animations
- No audio implementation
- Performance impact with multiple Mindmeld users

## File Structure
```
Source/blackhole/
├── Components/
│   ├── Attributes/      # Health, Stamina, WillPower
│   └── Abilities/       # All ability implementations
├── Player/              # Player character class
├── Enemy/               # Base enemy and variants
├── UI/                  # HUD implementation
└── Core/                # Game mode
```