# Blackhole - Component-Based Third-Person Action Game

## Component Hierarchy Diagram

```
ActorComponent (UE5 Base)
├── UAttributeComponent (Base)
│   ├── UIntegrityComponent (Health - No Regen)
│   ├── UStaminaComponent (10/sec Regen)
│   └── UWillPowerComponent (Hack Resource - No Regen)
│
└── UAbilityComponent (Base)
    ├── USlashAbilityComponent (20 dmg, 10 stamina, 2s CD)
    ├── USystemFreezeAbilityComponent (10 will, 2s stun, 3s CD)
    ├── UKillAbilityComponent (40 will, instant kill, 5s CD)
    ├── USmashAbilityComponent (10 dmg, 1.5s CD)
    ├── UBlockComponent (Block physical, 2s CD)
    ├── UDodgeComponent (Dodge physical, 2s CD)
    └── UMindmeldComponent (1 will/sec drain on sight)

Character (AActor)
├── ABlackholePlayerCharacter
│   ├── IntegrityComponent
│   ├── StaminaComponent
│   ├── WillPowerComponent
│   ├── SlashAbilityComponent
│   ├── SystemFreezeAbilityComponent
│   └── KillAbilityComponent
│
└── ABaseEnemy
    ├── IntegrityComponent
    ├── ACombatEnemy
    │   ├── SmashAbilityComponent
    │   ├── BlockComponent
    │   └── DodgeComponent
    └── AHackerEnemy
        └── MindmeldComponent
```

## How to Add New Enemy Types

1. **Create a new enemy class** inheriting from `ABaseEnemy`:
   ```cpp
   // MyNewEnemy.h
   #pragma once
   #include "Enemy/BaseEnemy.h"
   #include "MyNewEnemy.generated.h"
   
   UCLASS()
   class BLACKHOLE_API AMyNewEnemy : public ABaseEnemy
   {
       GENERATED_BODY()
   public:
       AMyNewEnemy();
   protected:
       virtual void UpdateAIBehavior(float DeltaTime) override;
   };
   ```

2. **Add desired ability components** in the constructor:
   ```cpp
   // MyNewEnemy.cpp
   AMyNewEnemy::AMyNewEnemy()
   {
       // Add any combination of ability components
       MyAbility = CreateDefaultSubobject<UAbilityComponent>(TEXT("MyAbility"));
   }
   ```

3. **Implement AI behavior** in `UpdateAIBehavior()`:
   ```cpp
   void AMyNewEnemy::UpdateAIBehavior(float DeltaTime)
   {
       // Custom AI logic
       if (TargetActor && ShouldAttack())
       {
           MyAbility->Execute();
       }
   }
   ```

## How to Add New Abilities

1. **Create a new ability class** inheriting from `UAbilityComponent`:
   ```cpp
   // MyNewAbilityComponent.h
   #pragma once
   #include "Components/Abilities/AbilityComponent.h"
   #include "MyNewAbilityComponent.generated.h"
   
   UCLASS()
   class BLACKHOLE_API UMyNewAbilityComponent : public UAbilityComponent
   {
       GENERATED_BODY()
   public:
       UMyNewAbilityComponent();
       virtual void Execute() override;
       virtual bool CanExecute() const override;
   };
   ```

2. **Set ability parameters** in constructor:
   ```cpp
   UMyNewAbilityComponent::UMyNewAbilityComponent()
   {
       Cost = 15.0f;        // Resource cost
       Cooldown = 2.5f;     // Cooldown in seconds
       Range = 500.0f;      // Ability range
   }
   ```

3. **Implement ability logic** in `Execute()`:
   ```cpp
   void UMyNewAbilityComponent::Execute()
   {
       if (!CanExecute()) return;
       
       Super::Execute(); // Starts cooldown
       
       // Your ability logic here
       // - Use line traces for targeting
       // - Apply effects to targets
       // - Consume resources
   }
   ```

4. **Add resource checks** in `CanExecute()`:
   ```cpp
   bool UMyNewAbilityComponent::CanExecute() const
   {
       if (!Super::CanExecute()) return false;
       
       // Check resource availability
       if (StaminaComponent && !StaminaComponent->HasEnoughStamina(Cost))
           return false;
           
       return true;
   }
   ```

## Current Implementation Status

### ✅ Completed Features:
- **Core Systems:**
  - Component-based architecture
  - Base attribute system with regeneration support
  - Base ability system with cooldowns
  - Enhanced Input System integration
  - First/Third person camera toggle
  
- **Attributes:**
  - Integrity (Health) - No regeneration, supports blocking
  - Stamina - 10/sec regeneration
  - WillPower - No regeneration
  
- **Player Abilities:**
  - Slash (LMB) - Physical melee attack
  - System Freeze (1) - Stuns enemies only
  - Kill (2) - Instant kill enemies only
  - Camera Toggle (H) - Switch between 1st/3rd person
  
- **Enemy Types:**
  - Base Enemy - Ragdoll physics on death
  - Combat Enemy - Balanced with all combat abilities
  - Agile Enemy - Fast, can dodge but not block
  - Tank Enemy - Slow, high health, can block but not dodge
  - Hacker Enemy - Drains player's will power with Mindmeld
  
- **Enemy Abilities:**
  - Smash - Basic melee attack
  - Block - 50% damage reduction
  - Dodge - Quick movement dash
  - Mindmeld - Line-of-sight will drain (player only)
  
- **UI System:**
  - Player attribute bars with color coding
  - Ability cooldown indicators
  - Crosshair
  - Target enemy information display
  
- **Game Mode:**
  - Default player and HUD setup

### 🔧 Implementation Notes:
- All numeric values are exposed as `UPROPERTY(EditAnywhere)`
- Proper UE5.5 naming conventions followed
- Component tick enabled only where necessary
- Debug visualization and logging included
- Hack abilities restricted to enemy targets only
- All enemies tagged with "Enemy" for identification

## Future Features Placeholder

### Planned Enhancements:
- **Save System** - Checkpoint and progress saving
- **Level Progression** - Experience and upgrade system
- **Additional Abilities** - Expand player and enemy arsenals
- **Environmental Hazards** - Interactive level elements
- **Multiplayer Support** - Co-op or PvP modes
- **Audio System** - SFX and dynamic music
- **Particle Effects** - Visual feedback for abilities
- **Animation System** - Ability animations and hit reactions
- **AI Director** - Dynamic enemy spawning
- **Objective System** - Mission tracking

### Technical Improvements:
- Object pooling for projectiles
- AI behavior trees
- Network replication
- Performance optimization
- Modular ability data assets
- Localization support

## Setup Instructions:

1. Place all source files in your Unreal Engine project's Source folder
2. Regenerate project files
3. Compile the project
4. Set `ABlackholeGameMode` as the default game mode
5. Create a level with player start
6. Add enemy actors to test combat

## Input Mappings Required:

Add these to Project Settings > Input:
- **Action Mappings:**
  - "Slash" → Left Mouse Button
  - "SystemFreeze" → 1 Key
  - "Kill" → 2 Key
  - "Jump" → Spacebar
  
- **Axis Mappings:**
  - "MoveForward" → W (+1.0), S (-1.0)
  - "MoveRight" → D (+1.0), A (-1.0)
  - "Turn" → Mouse X
  - "LookUp" → Mouse Y
  - "TurnRate" → Right/Left Arrow
  - "LookUpRate" → Up/Down Arrow