# Blackhole Project Changelog

## [Session 2025-01-02] - Major Updates

### Added
- **Enhanced Input System** - Replaced old input system with UE5's Enhanced Input
  - Created input actions for all player controls
  - Implemented proper input binding in C++
  - Ready for runtime remapping

- **Camera Toggle System**
  - Press H to switch between first and third person
  - Smooth transitions with proper settings for each mode
  - Character mesh visibility handled automatically

- **New Enemy Types**
  - **AgileEnemy**: Can dodge and attack, but cannot block
  - **TankEnemy**: Can block and attack, but cannot dodge (slower, more health)

- **Ragdoll Death System**
  - All enemies ragdoll when Integrity reaches 0
  - Physics impulse applied for dramatic effect
  - Bodies auto-cleanup after 10 seconds

### Fixed
- **Mindmeld Ability Not Working**
  - Added `PrimaryComponentTick.bCanEverTick = true` to MindmeldComponent
  - Fixed line of sight calculations (now properly ignores actors)
  - Raised trace points to eye level for better detection

- **Hack Ability Targeting**
  - SystemFreeze and Kill now only work on enemies
  - Added "Enemy" tag to all enemies for identification
  - Type checking with `IsA<ABaseEnemy>()` as fallback

- **HackerEnemy AI**
  - Now properly targets only the player
  - Won't attempt to mindmeld other enemies
  - Better line of sight maintenance

### Changed
- **Block Mechanic**
  - Now reduces damage by 50% when active
  - IntegrityComponent checks for BlockComponent status

- **Debug Visualization**
  - Mindmeld line now draws at eye level
  - Thicker debug lines for better visibility
  - Added extensive debug logging

## [Session 2025-01-02 - Initial] - Project Setup

### Added
- **Complete Project Structure**
  - Organized folder hierarchy
  - Component-based architecture implementation

- **Base Component System**
  - AttributeComponent with regeneration support
  - AbilityComponent with cooldown management

- **All Planned Components**
  - 3 Attribute components (Integrity, Stamina, WillPower)
  - 7 Ability components (Slash, SystemFreeze, Kill, Smash, Block, Dodge, Mindmeld)

- **Character Classes**
  - BlackholePlayerCharacter with all player abilities
  - BaseEnemy with basic AI framework
  - CombatEnemy and HackerEnemy variants

- **UI System**
  - Complete HUD with attribute bars
  - Cooldown indicators
  - Target information display

- **Game Mode**
  - Basic setup linking player and HUD

### Technical Notes
- All classes follow UE5.5 naming conventions
- UPROPERTY macros properly configured
- Forward declarations used to minimize compile times
- Proper include guards and generated body macros