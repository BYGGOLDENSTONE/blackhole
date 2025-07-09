# Blackhole - Combo System Design
**Date**: 2025-07-09  
**System**: Advanced Combat Combos

## Core Design Philosophy

Combos should:
1. **Reward Skill**: Higher execution difficulty = greater rewards
2. **Feel Different**: Each path (Hacker/Forge) has unique combo properties
3. **Resource Efficient**: Combos cost less resources than individual abilities
4. **Visually Distinct**: Clear visual feedback for successful combos
5. **Strategic Value**: Each combo serves a tactical purpose

---

## Combo Definitions

### 1. Dash + Slash → "Phantom Strike"

**Input Window**: 0.5 seconds  
**Execution**: Dash followed immediately by Slash

#### Hacker Version - "Phase Cutter"
- **Effect**: Teleport behind target and deliver critical backstab
- **Damage**: 150% normal slash damage
- **Special**: Applies "Disrupted" debuff (-50% turn speed for 2s)
- **Resource Cost**: 5 Stamina (50% off normal cost)
- **WP Generation**: +10 (reduced from +15)
- **Visual**: Blue afterimage trail, digital glitch on enemy

#### Forge Version - "Meteor Rush"
- **Effect**: Charge through enemies with flaming trail
- **Damage**: 100% slash + burn DoT (20 damage/sec for 3s)
- **Special**: Pierces through multiple enemies
- **Resource Cost**: 5 Stamina (50% off)
- **Heat Generation**: +15 (reduced from +20)
- **Visual**: Fire trail, enemies ignite on contact

```cpp
// Implementation approach
void ABlackholePlayerCharacter::OnDashComplete()
{
    // Start combo window
    bDashComboWindow = true;
    GetWorld()->GetTimerManager().SetTimer(
        ComboWindowTimer,
        [this]() { bDashComboWindow = false; },
        0.5f,
        false
    );
}

void USlashAbilityComponent::Execute()
{
    if (Character->bDashComboWindow)
    {
        ExecutePhantomStrike();
        return;
    }
    // Normal slash
}
```

### 2. Jump + Slash → "Aerial Rave"

**Input Window**: 0.3 seconds after jump  
**Execution**: Slash while airborne

#### Hacker Version - "Sky Blade"
- **Effect**: Downward slash creating shockwave on landing
- **Damage**: 100% slash + 50 AoE damage
- **Special**: Shockwave slows enemies by 30% for 2s
- **Resource Cost**: 8 Stamina (combined discount)
- **WP Generation**: +12
- **Visual**: Cyan energy wave on impact

#### Forge Version - "Hammer Drop"
- **Effect**: Plunging attack with ground crater
- **Damage**: 200% slash damage in AoE
- **Special**: Knockback all nearby enemies
- **Resource Cost**: 8 Stamina
- **Heat Generation**: +20
- **Visual**: Molten impact crater, screen shake

```cpp
void USlashAbilityComponent::Execute()
{
    if (Character->GetCharacterMovement()->IsFalling())
    {
        ExecuteAerialRave();
        return;
    }
}
```

### 3. Jump + Dash + Slash → "Tempest Blade"

**Input Window**: 0.3s → 0.3s  
**Execution**: Complex aerial maneuver  
**Difficulty**: High

#### Hacker Version - "Quantum Assassinate"
- **Effect**: Teleport to up to 3 enemies in sequence with slashes
- **Damage**: 80% per hit (240% total if all 3 hit)
- **Special**: Brief invulnerability during teleports
- **Resource Cost**: 15 Stamina (major discount)
- **WP Generation**: +20 (instead of +40)
- **Visual**: Multiple afterimages, time dilation effect

#### Forge Version - "Phoenix Dive"
- **Effect**: Aerial spin creating fire tornado
- **Damage**: Continuous 50 damage/tick for 2s
- **Special**: Pulls enemies into center
- **Resource Cost**: 15 Stamina
- **Heat Generation**: +30
- **Visual**: Fire tornado, enemies lifted

```cpp
// Track combo state
enum class EComboState
{
    None,
    JumpInitiated,
    JumpDashComplete,
    ComboComplete
};

void ABlackholePlayerCharacter::UpdateComboState()
{
    switch (CurrentComboState)
    {
        case EComboState::JumpInitiated:
            if (bDashExecuted)
            {
                CurrentComboState = EComboState::JumpDashComplete;
                StartComboWindow(0.3f);
            }
            break;
    }
}
```

### 4. Slash + Slash → "Blade Dance"

**Input Window**: 0.8 seconds  
**Execution**: Rhythm-based slashing

#### Progressive Chain System
Instead of just 2 slashes, this becomes a rhythm combo up to 5 hits:

**Hit 1-2**: Normal damage  
**Hit 3**: 125% damage + mini stun (0.2s)  
**Hit 4**: 150% damage + armor break (-20%)  
**Hit 5**: 200% damage + special finisher

#### Hacker Version - "Data Storm"
- **Finisher**: 360° data wave that corrupts all enemies
- **Effect**: -5 WP per enemy hit
- **Special**: Each hit is faster than the last
- **Visual**: Increasing digital effects per hit

#### Forge Version - "Forge Fury"
- **Finisher**: Massive overhead slam
- **Effect**: -10 Heat on completion
- **Special**: Each hit does more damage
- **Visual**: Weapon glows brighter per hit

```cpp
class USlashComboTracker : public UObject
{
    int32 CurrentHitCount = 0;
    float LastHitTime = 0.0f;
    float ComboWindow = 0.8f;
    
    void RegisterHit()
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastHitTime <= ComboWindow)
        {
            CurrentHitCount++;
            ApplyComboEffects(CurrentHitCount);
            
            // Tighten window for rhythm
            ComboWindow *= 0.9f;
        }
        else
        {
            ResetCombo();
        }
        
        LastHitTime = CurrentTime;
    }
};
```

---

## Resource Economy

### Combo Discounts
- **2-ability combo**: 25% resource discount
- **3-ability combo**: 40% resource discount
- **Perfect timing**: Additional 10% discount

### WP/Heat Management
- Combos generate less corruption/heat than individual abilities
- Successful combos can cleanse WP or vent Heat
- Failed combos cost full resources

---

## Visual Feedback System

### Combo Indicators
1. **Input Buffer Visual**: Show remaining combo window as shrinking circle
2. **Success Flash**: Screen edge flash on successful combo
3. **Combo Counter**: Stylish UI element showing current combo
4. **Trail Effects**: Unique particle trails for each combo

### Audio Design
- **Input Success**: Distinct "ching" per successful input
- **Combo Complete**: Satisfying crescendo sound
- **Perfect Timing**: Special audio cue
- **Music Sync**: Combos align with beat for rhythm bonus

---

## Advanced Combo Mechanics

### 1. Combo Canceling
Allow certain abilities to cancel recovery frames:
```cpp
bool UAbilityComponent::CanCancelIntoCombo()
{
    return CurrentRecoveryTime > 0.0f && 
           CurrentRecoveryTime < MaxRecoveryTime * 0.7f;
}
```

### 2. Directional Inputs
Add directional variants:
- **Forward + Slash**: Lunge attack
- **Back + Slash**: Defensive spin
- **Side + Slash**: Wide sweep

### 3. Resource Combos
Combine different ability types:
- **Firewall Breach + Slash**: "Exploit Strike" (true damage)
- **Heat Shield + Hammer Strike**: "Molten Fortress" (AoE reflect)

### 4. Environmental Combos
- **Near Wall + Dash + Slash**: Wall bounce attack
- **Near Ledge + Jump + Slash**: Extended aerial time

---

## Implementation Priority

### Phase 1: Core System (Day 1-2)
1. Implement combo state machine
2. Create input buffer system
3. Add basic 2-hit combos

### Phase 2: Visual Polish (Day 3)
4. Add particle effects
5. Implement hit stop on combo hits
6. Create combo UI

### Phase 3: Advanced Features (Day 4-5)
7. Add 3-hit combo
8. Implement rhythm system
9. Add combo canceling

---

## Balancing Considerations

### Skill vs Reward
- Easy combos: Small damage boost, resource efficiency
- Hard combos: Major damage, unique effects, style points

### Anti-Spam
- Diminishing returns on repeated same combo
- Combo variety bonus (using different combos = more rewards)

### PvE Balance
- Enemies should react to combos (block, dodge, interrupt)
- Elite enemies might have combo-breaker moves
- Bosses could require specific combos to break shields

---

## Technical Implementation

### Combo Detection System
```cpp
UCLASS()
class UComboSystem : public UActorComponent
{
    GENERATED_BODY()
    
private:
    // Circular buffer for input history
    TCircularBuffer<FAbilityInput> InputHistory;
    
    // Registered combo patterns
    TArray<FComboPattern> RegisteredCombos;
    
    // Active combo window
    float ComboWindowRemaining;
    
public:
    void RegisterInput(EAbilityType AbilityType);
    bool CheckForCombo();
    void ExecuteCombo(const FComboPattern& Combo);
};
```

### Combo Data Structure
```cpp
USTRUCT()
struct FComboPattern
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere)
    TArray<EAbilityType> RequiredInputs;
    
    UPROPERTY(EditAnywhere)
    TArray<float> TimingWindows;
    
    UPROPERTY(EditAnywhere)
    TSubclassOf<UComboEffect> EffectClass;
    
    UPROPERTY(EditAnywhere)
    float ResourceDiscount = 0.25f;
};
```

---

## Player Mastery Path

### Combo Grades
- **C**: Basic execution
- **B**: Good timing
- **A**: Perfect timing
- **S**: Perfect timing + style (no damage taken)
- **SSS**: Full combo chain with variety

### Rewards for Mastery
- Unlock new combo effects at higher grades
- Cosmetic changes (cooler VFX at S rank)
- Resource bonuses for perfect execution

---

## Conclusion

This combo system adds depth without complexity. Players can enjoy the game using basic attacks, but mastery reveals a rich combat system with satisfying rewards for skilled play. The path-specific variations ensure both Hacker and Forge players have unique combo experiences that reinforce their chosen playstyle.