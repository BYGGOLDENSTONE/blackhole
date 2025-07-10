# Blackhole - Combo System Documentation
**System**: Simple Input-Based Combo Detection
**Status**: Fully Implemented

## Core Design Philosophy

Combos should:
1. **Reward Skill**: Higher execution difficulty = greater rewards
2. **Component-Based**: Each combo is a self-contained component with Blueprint-editable parameters
3. **Resource Efficient**: Combos cost less resources than individual abilities
4. **Visually Distinct**: Clear visual feedback for successful combos
5. **Strategic Value**: Each combo serves a tactical purpose
6. **Stability First**: All crash issues resolved, proper cleanup implemented

---

## System Architecture

### Simple Input Detection
The combo system uses straightforward last-input tracking in the player character:

```cpp
// In player character
enum class ELastAbilityUsed : uint8
{
    None,
    Dash,
    Jump,
    Slash
};

ELastAbilityUsed LastAbilityUsed = ELastAbilityUsed::None;
float LastAbilityTime = 0.0f;
float ComboWindowDuration = 0.5f; // Blueprint-editable
```

When slash is used:
1. Check if dash or jump was used within combo window
2. If yes, execute corresponding combo component instead of normal slash
3. Reset tracking after combo execution

### Key Features:
- **Simple Detection**: Last-input tracking with timing window
- **Component Execution**: Each combo is a separate ability component
- **Blueprint Editable**: All parameters exposed to designers
- **Time Slow System**: Real-time based using FPlatformTime::Seconds()
- **HitStop Disabled**: Avoided conflicts by disabling HitStop during combos

---

## Implemented Combos

### 1. Dash + Slash → "Phantom Strike" (DashSlashCombo)

**Component**: `UDashSlashCombo`  
**Input Window**: 0.5 seconds (Blueprint editable)  
**Execution**: Dash followed immediately by Slash

#### Hacker Version - "Phase Cutter"
- **Effect**: Teleport behind target and deliver critical backstab
- **Base Damage**: 150% normal slash damage
- **Backstab Multiplier**: 2.0x (Blueprint editable)
- **Teleport Distance**: 150 units (Blueprint editable)
- **Time Slow**: 0.1 scale for 0.25s (Blueprint editable)
- **Resource Cost**: 20 Stamina, 25 WP
- **Visual**: Blue afterimage trail, digital glitch on enemy

#### Blueprint Parameters:
```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike")
float TeleportDistance = 150.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike")
float BackstabDamageMultiplier = 2.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Strike")
bool bAutoRotateToTarget = true;
```

### 2. Jump + Slash → "Aerial Rave" (JumpSlashCombo)

**Component**: `UJumpSlashCombo`  
**Input Window**: 0.3 seconds after jump (Blueprint editable)  
**Execution**: Slash while airborne

#### Hacker Version - "Sky Blade"
- **Effect**: Downward slash creating shockwave on landing
- **Base Damage**: 125% slash damage
- **Shockwave Radius**: 300 units (Blueprint editable)
- **Shockwave Damage**: 50 with distance falloff (Blueprint editable)
- **Downward Force**: 1000 units/s (Blueprint editable)
- **Resource Cost**: 20 Stamina, 25 WP
- **Visual**: Cyan energy wave on impact

#### Blueprint Parameters:
```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave")
float ShockwaveRadius = 300.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave")
float ShockwaveDamage = 50.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerial Rave")
float DownwardForce = 1000.0f;
```

### 3. Jump + Dash + Slash → "Tempest Blade" (Not Yet Implemented)

**Status**: Planned for future implementation as component  
**Input Window**: 0.3s → 0.3s  
**Difficulty**: High

---

## Resource Economy

### Combo Resource Costs
All combo components inherit from `UComboAbilityComponent` with these default costs:
- **Base Stamina Cost**: 20 (editable per combo)
- **Base WP Cost**: 25 (editable per combo)
- **Cooldown**: 0.5s (editable per combo)

### Resource Management
- Combos cost less than executing abilities separately
- Failed combo attempts still consume resources
- Blueprint parameters allow fine-tuning per combo

---

## Visual Feedback System

### Per-Component Visual Settings
Each combo component includes customizable visual parameters:

```cpp
// Trail color for combo execution
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
FLinearColor ComboTrailColor;

// Particle effects
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
UParticleSystem* ComboParticle;

// Sound effects
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
USoundBase* ComboSound;

// Camera shake intensity
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
float CameraShakeScale = 1.0f;
```

### Hit Stop Integration
Combos automatically apply hit stop based on damage dealt:
- Light hit stop: < 25 damage
- Medium hit stop: 25-50 damage
- Heavy hit stop: 50-100 damage
- Critical hit stop: > 100 damage

---

## Implementation Details

### Player Character Detection
In `UseAbilitySlot1()` (slash ability):

```cpp
float CurrentTime = GetWorld()->GetTimeSeconds();
float TimeSinceLastAbility = CurrentTime - LastAbilityTime;

if (TimeSinceLastAbility <= ComboWindowDuration)
{
    if (LastAbilityUsed == ELastAbilityUsed::Dash && DashSlashCombo->CanExecute())
    {
        DashSlashCombo->Execute();
        LastAbilityUsed = ELastAbilityUsed::None;
        return; // Don't execute normal slash
    }
    else if (LastAbilityUsed == ELastAbilityUsed::Jump && JumpSlashCombo->CanExecute())
    {
        JumpSlashCombo->Execute();
        LastAbilityUsed = ELastAbilityUsed::None;
        return;
    }
}
// No combo - execute normal slash
SlashAbility->Execute();
```

---

## Adding New Combos

1. Create a new C++ class inheriting from `UComboAbilityComponent`
2. Override `ExecuteCombo()` with combo logic
3. Add component to player character
4. Add detection case in player's slash function
5. Configure parameters in Blueprint

---

## Time Slow System

### Implementation:
- Uses `FPlatformTime::Seconds()` for real-time duration tracking
- Automatically resets after duration expires via TickComponent
- HitStop disabled during combos to avoid conflicts
- Proper cleanup in EndPlay to prevent PIE issues

```cpp
void ApplyTimeSlow()
{
    UGameplayStatics::SetGlobalTimeDilation(World, TimeSlowScale);
    TimeSlowEndTime = FPlatformTime::Seconds() + TimeSlowDuration;
    bIsTimeSlowActive = true;
}
```