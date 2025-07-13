# Blackhole AI State Machine Documentation

## Overview

The Blackhole project uses a modular, extensible state machine architecture for enemy AI. This system replaces traditional if/else chains with a clean, maintainable structure that's easy to debug and extend.

## Architecture

### Core Components

```
UEnemyStateMachine (Base)
├── UCombatEnemyStateMachine
├── UAgileEnemyStateMachine  
├── UTankEnemyStateMachine
└── UHackerEnemyStateMachine

UEnemyStateBase (Abstract)
├── UIdleState
├── UAlertState
├── UChaseState
├── UCombatState (Base)
│   ├── UAgileCombatState
│   ├── UTankCombatState
│   ├── UVersatileCombatState
│   └── UHackerCombatState
├── URetreatState
└── UChannelingState
```

### State Definitions

```cpp
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
    None,
    Idle,       // Default patrol/waiting state
    Alert,      // Detected something suspicious
    Chase,      // Actively pursuing target
    Combat,     // In combat range, attacking
    Retreat,    // Running away (low health)
    Defensive,  // Blocking/dodging focus
    Channeling, // Casting abilities (Hacker)
    Dead        // Death state
};
```

## Key Features

### 1. Modular State System
Each state is a separate class with clear responsibilities:
- **Enter()**: Called when entering the state
- **Exit()**: Called when leaving the state  
- **Update()**: Called every tick while in state
- **CanTransitionTo()**: Validates state transitions

### 2. Configurable AI Parameters
```cpp
struct FEnemyAIParameters
{
    // Health thresholds
    float RetreatHealthPercent = 0.3f;
    float DefensiveHealthPercent = 0.5f;
    
    // Distance thresholds
    float AttackRange = 200.0f;
    float ChaseRange = 1000.0f;
    float SightRange = 1500.0f;
    
    // Timing parameters
    float ReactionTime = 0.3f;
    float SearchDuration = 5.0f;
    
    // Combat parameters  
    float DodgeChance = 0.2f;
    float BlockChance = 0.3f;
    float AttackCooldown = 2.0f;
};
```

### 3. Multi-Height Detection
Enemies check multiple heights for wall-running players:
```cpp
TArray<FVector> TargetPositions = {
    TargetLoc,                         // Normal
    TargetLoc + FVector(0, 0, 100),  // Wall run
    TargetLoc + FVector(0, 0, 200),  // High wall run
    TargetLoc - FVector(0, 0, 50)   // Crouching
};
```

### 4. Player Action Reactions
State machines respond to player actions:
- `NotifyPlayerDashed()`: React to player dash
- `NotifyPlayerAttacking()`: Defensive responses
- `NotifyPlayerUltimateUsed()`: Evasive actions
- `NotifyDamageTaken()`: Combat adjustments

## Enemy-Specific Behaviors

### Tank Enemy
- **Movement**: Slow (300 units/s), heavy (2x mass)
- **States**: No defensive state, uses block in combat
- **Special**: Ground slam with area damage and knockback
- **Configurable**: GroundSlamRadius, DamageMultiplier, KnockbackForce

### Agile Enemy  
- **Movement**: Fast (600 units/s), nimble turning
- **States**: Circle strafe when dash on cooldown
- **Special**: Dash behind player for backstab
- **Configurable**: MovementSpeed, AttackSpeedMultiplier, DashCooldown

### Combat Enemy
- **Movement**: Balanced (400 units/s)
- **States**: All standard states
- **Special**: Can block and dodge
- **Focus**: Versatile melee combat

### Hacker Enemy
- **Movement**: Cautious (350 units/s)
- **States**: Uses channeling for abilities
- **Special**: Ranged mindmeld attacks
- **Focus**: Maintain safe distance

## Creating New Enemy Types

### 1. Create State Machine Class
```cpp
UCLASS()
class UMyEnemyStateMachine : public UEnemyStateMachine
{
    GENERATED_BODY()
    
protected:
    virtual void BeginPlay() override;
    virtual void CreateDefaultStates() override;
    
    void SetupMyEnemyParameters();
};
```

### 2. Implement Setup
```cpp
void UMyEnemyStateMachine::BeginPlay()
{
    Super::BeginPlay();
    SetupMyEnemyParameters();
    Initialize(); // Must call after parameters
}

void UMyEnemyStateMachine::SetupMyEnemyParameters()
{
    FEnemyAIParameters Params;
    Params.AttackRange = 300.0f;
    Params.DodgeChance = 0.5f;
    // ... configure as needed
    SetAIParameters(Params);
}
```

### 3. Create Custom States
```cpp
void UMyEnemyStateMachine::CreateDefaultStates()
{
    // Create standard states
    RegisterState(EEnemyState::Idle, NewObject<UIdleState>(this));
    RegisterState(EEnemyState::Chase, NewObject<UChaseState>(this));
    
    // Create custom combat state
    RegisterState(EEnemyState::Combat, NewObject<UMyCombatState>(this));
}
```

## Creating Custom States

### 1. Inherit from Base
```cpp
UCLASS()
class UMyCombatState : public UCombatState
{
    GENERATED_BODY()
    
protected:
    virtual void InitializeCombatActions(ABaseEnemy* Enemy) override;
    virtual void ExecuteCombatAction(ABaseEnemy* Enemy, 
        UEnemyStateMachine* StateMachine, 
        const FString& ActionName) override;
};
```

### 2. Define Combat Actions
```cpp
void UMyCombatState::InitializeCombatActions(ABaseEnemy* Enemy)
{
    // AddCombatAction(Name, Weight, Cooldown, MinRange, MaxRange)
    AddCombatAction(TEXT("Slash"), 3.0f, 1.0f, 0.0f, 200.0f);
    AddCombatAction(TEXT("Spin"), 2.0f, 3.0f, 0.0f, 300.0f);
    AddCombatAction(TEXT("Charge"), 1.0f, 5.0f, 300.0f, 800.0f);
}
```

### 3. Implement Actions
```cpp
void UMyCombatState::ExecuteCombatAction(ABaseEnemy* Enemy, 
    UEnemyStateMachine* StateMachine, const FString& ActionName)
{
    if (ActionName == TEXT("Slash"))
    {
        // Execute slash attack
        if (UMySlashAbility* Slash = Enemy->FindComponentByClass<UMySlashAbility>())
        {
            Slash->Execute();
        }
        StartAbilityCooldown(Enemy, TEXT("Slash"), 1.0f);
    }
}
```

## State Transition Logic

### Idle → Alert
```cpp
// In IdleState::Update()
if (StateMachine->GetTarget() && Distance < SightRange)
{
    if (StateMachine->HasLineOfSight())
    {
        StateMachine->ChangeState(EEnemyState::Alert);
    }
}
```

### Alert → Chase/Combat
```cpp
// In AlertState::Update()
if (TimeInState > ReactionTime)
{
    if (Distance <= AttackRange)
    {
        StateMachine->ChangeState(EEnemyState::Combat);
    }
    else if (Distance <= ChaseRange)
    {
        StateMachine->ChangeState(EEnemyState::Chase);
    }
}
```

### Combat → Retreat
```cpp
// In CombatState::Update()
float HealthPercent = IntegrityComponent->GetCurrentValue() / 
                     IntegrityComponent->GetMaxValue();
                     
if (HealthPercent < AIParams.RetreatHealthPercent)
{
    StateMachine->ChangeState(EEnemyState::Retreat);
}
```

## Debugging

### Visual Debug
```cpp
// In state Update() methods
DrawDebugString(GetWorld(), 
    Enemy->GetActorLocation() + FVector(0, 0, 100),
    *FString::Printf(TEXT("State: %s"), *GetStateName()),
    nullptr, FColor::Yellow, DeltaTime);
```

### Logging
```cpp
UE_LOG(LogTemp, Warning, TEXT("%s: Entered %s state"), 
    *Enemy->GetName(), *UEnum::GetValueAsString(NewState));
```

### Common Issues

1. **State Not Changing**
   - Check CanTransitionTo() implementation
   - Verify state is registered in CreateDefaultStates()
   - Check transition conditions in Update()

2. **Null State Object**
   - Ensure Initialize() is called after BeginPlay()
   - Check state registration order
   - Verify NewObject calls succeed

3. **Performance Issues**
   - Reduce tick rate: `PrimaryComponentTick.TickInterval = 0.1f`
   - Use timers for expensive checks
   - Cache frequently accessed values

## Best Practices

1. **State Initialization**
   - Always call parent BeginPlay()
   - Set parameters before Initialize()
   - Register all required states

2. **Combat Actions**
   - Use weight system for variety
   - Respect cooldowns
   - Consider range requirements

3. **Performance**
   - Use tick intervals appropriately
   - Cache target references
   - Minimize per-frame calculations

4. **Extensibility**
   - Keep states focused and single-purpose
   - Use parameters over hardcoded values
   - Document state transitions

## Example: Adding New Ability

1. **Create Ability Component**
```cpp
UCLASS()
class ULightningStrikeComponent : public UAbilityComponent
{
    // Implementation
};
```

2. **Add to Enemy**
```cpp
// In enemy constructor
LightningAbility = CreateDefaultSubobject<ULightningStrikeComponent>(TEXT("Lightning"));
```

3. **Add Combat Action**
```cpp
// In custom combat state
AddCombatAction(TEXT("Lightning"), 1.5f, 4.0f, 500.0f, 1500.0f);
```

4. **Execute in State**
```cpp
if (ActionName == TEXT("Lightning"))
{
    if (ULightningStrikeComponent* Lightning = Enemy->FindComponentByClass<ULightningStrikeComponent>())
    {
        Lightning->Execute();
        StartAbilityCooldown(Enemy, TEXT("Lightning"), 4.0f);
    }
}
```

## Configurable Properties Reference

### AgileEnemy
- `MovementSpeed` (600.0f): Base movement speed
- `AttackSpeedMultiplier` (1.5f): Attack animation speed
- `DashCooldown` (2.0f): Time between dashes
- `DashBehindDistance` (250.0f): Distance behind target
- `AttackRange` (100.0f): Dagger-like short range

### TankEnemy
- `GroundSlamRadius` (1000.0f): Area effect radius
- `GroundSlamDamageMultiplier` (2.0f): Damage multiplier
- `GroundSlamKnockbackForce` (1000.0f): Push force
- `AttackRange` (150.0f): Melee range
- `MovementSpeed` (300.0f): Slow heavy movement

## Future Improvements

1. **Behavior Trees Integration**
   - States could trigger behavior tree tasks
   - More complex decision making
   - Visual scripting support

2. **Group Coordination**
   - Enemies communicate state changes
   - Coordinated attacks
   - Formation movement

3. **Dynamic Difficulty**
   - Adjust parameters based on player skill
   - Scale reaction times
   - Modify ability usage

4. **Animation Integration**
   - State-based animation blueprints
   - Blend spaces for movement
   - Procedural look-at targets