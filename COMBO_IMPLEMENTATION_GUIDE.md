# Combo Implementation Guide

## Overview
The combo system uses simple last-input tracking in the player character to detect combo sequences. When a valid combo is detected, it executes a dedicated combo component instead of the normal ability.

## How Combo Detection Works

### 1. Input Tracking
The player character tracks the last ability used and when it was used:

```cpp
// In ABlackholePlayerCharacter
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

### 2. Ability Registration
When dash or jump abilities are used, they register themselves:

```cpp
void ABlackholePlayerCharacter::UseDash()
{
    if (HackerDashAbility->CanExecute())
    {
        HackerDashAbility->Execute();
        LastAbilityUsed = ELastAbilityUsed::Dash;
        LastAbilityTime = GetWorld()->GetTimeSeconds();
    }
}
```

### 3. Combo Detection
When slash is used, it checks for valid combos:

```cpp
void ABlackholePlayerCharacter::UseAbilitySlot1() // Slash
{
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
    
    // No combo detected - execute normal slash
    SlashAbility->Execute();
}
```

## Time Slow System

### Implementation
The combo base class (`UComboAbilityComponent`) handles time slow effects:

1. **Real-Time Tracking**: Uses `FPlatformTime::Seconds()` instead of game time
2. **Automatic Reset**: TickComponent checks when time slow should end
3. **Proper Cleanup**: EndPlay ensures time is reset when leaving PIE

```cpp
void UComboAbilityComponent::ApplyTimeSlow()
{
    // Apply time dilation
    UGameplayStatics::SetGlobalTimeDilation(CachedWorld, TimeSlowScale);
    
    // Track when it should end (real time)
    TimeSlowEndTime = FPlatformTime::Seconds() + TimeSlowDuration;
    bIsTimeSlowActive = true;
}

void UComboAbilityComponent::TickComponent(float DeltaTime, ...)
{
    if (bIsTimeSlowActive)
    {
        float CurrentRealTime = FPlatformTime::Seconds();
        if (CurrentRealTime >= TimeSlowEndTime)
        {
            UGameplayStatics::SetGlobalTimeDilation(CachedWorld, 1.0f);
            bIsTimeSlowActive = false;
        }
    }
}
```

### HitStop Conflict Resolution
HitStop is disabled during combos to avoid time dilation conflicts:

```cpp
// In combo execution
// NOTE: Disabled hit stop to avoid conflicts with combo time slow
/*
if (UHitStopManager* HitStopMgr = CachedWorld->GetSubsystem<UHitStopManager>())
{
    ApplyHitStop(HitStopMgr, FinalDamage);
}
*/
```

## Blueprint Configuration

### Player Character Setup
1. Open the player character Blueprint
2. Find the Combos category in Details panel
3. Configure combo window duration (default 0.5s)

### Combo Component Parameters
Each combo component exposes these parameters:

**Base Parameters** (UComboAbilityComponent):
- `ComboWindowTime`: Detection window (not used by component, set on player)
- `TimeSlowScale`: Time dilation factor (0.1 = 10% speed)
- `TimeSlowDuration`: How long time slow lasts (real seconds)
- `DamageMultiplier`: Base damage multiplier for combo
- `ComboRange`: Detection range for targets

**DashSlashCombo Specific**:
- `TeleportDistance`: How far behind target to teleport
- `BackstabDamageMultiplier`: Additional damage for backstab
- `bAutoRotateToTarget`: Whether to face target after teleport

**JumpSlashCombo Specific**:
- `ShockwaveRadius`: AOE radius on landing
- `ShockwaveDamage`: Base shockwave damage
- `DownwardForce`: Force applied to slam down

## Creating New Combos

1. **Create Component Class**:
```cpp
UCLASS()
class UMyNewCombo : public UComboAbilityComponent
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My Combo")
    float MyCustomParameter = 1.0f;
    
    virtual void ExecuteCombo() override;
};
```

2. **Add to Player Character**:
```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combos")
UMyNewCombo* MyNewCombo;

// In constructor
MyNewCombo = CreateDefaultSubobject<UMyNewCombo>(TEXT("MyNewCombo"));
```

3. **Add Detection Logic**:
```cpp
// In UseAbilitySlot1() or wherever the combo ends
else if (LastAbilityUsed == ELastAbilityUsed::MyTrigger && MyNewCombo->CanExecute())
{
    MyNewCombo->Execute();
    LastAbilityUsed = ELastAbilityUsed::None;
    return;
}
```

## Known Issues and Solutions

### Issue: Time slow not resetting
**Solution**: The system now uses real-time tracking and has cleanup in EndPlay

### Issue: Time slow conflicts with HitStop
**Solution**: HitStop is disabled during combo execution

### Issue: Combos not triggering
**Check**:
1. Combo window duration is appropriate (default 0.5s)
2. Both abilities can execute (resources, cooldowns)
3. Input timing is within window

### Issue: Multiple combos triggering
**Solution**: Each combo execution resets the tracking immediately

## Debug Tips

1. **Enable Logging**: Check UE_LOG statements in player character
2. **Visualize Timing**: Add debug UI showing LastAbilityUsed and time remaining
3. **Blueprint Debugging**: Use Print String nodes to verify execution flow
4. **Time Slow Verification**: Log time dilation values before/after application

## Best Practices

1. **Timing Windows**: Keep combo windows reasonable (0.3-0.5s)
2. **Resource Costs**: Combos should cost less than individual abilities
3. **Visual Clarity**: Ensure combo effects are distinct from normal abilities
4. **Fail States**: Always reset tracking after combo attempts
5. **Cleanup**: Use EndPlay for any state that persists between PIE sessions