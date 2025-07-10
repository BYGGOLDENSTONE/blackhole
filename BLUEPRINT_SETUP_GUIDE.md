# Blackhole - Blueprint Setup Guide
**Date**: 2025-07-10  
**Purpose**: Guide for setting up the project in Unreal Engine after code cleanup

## Important Blueprint Changes Required

### 1. Player Blueprint (BP_BlackholePlayerCharacter)

#### Remove Old Components:
In the Components panel, remove if present:
- ComboTracker
- ComboSystem  
- ComboComponent

#### Verify Combo Components:
Ensure these components exist:
- **DashSlashCombo** (Phantom Strike)
- **JumpSlashCombo** (Aerial Rave)

#### Configure Combo Parameters:
For each combo component, you can now edit in the Details panel:

**DashSlashCombo Settings:**
- Combo Window Time: 0.5 (time allowed between inputs)
- Time Slow Scale: 0.3 (30% speed during execution)
- Time Slow Duration: 0.15 (seconds in real-time)
- Damage: 50
- Damage Multiplier: 1.5
- Combo Range: 300
- Teleport Distance: 150
- Backstab Damage Multiplier: 2.0

**JumpSlashCombo Settings:**
- Combo Window Time: 0.5
- Time Slow Scale: 0.3
- Time Slow Duration: 0.15
- Damage: 50
- Damage Multiplier: 1.25
- Combo Range: 300
- Shockwave Radius: 300
- Shockwave Damage: 50

### 2. Input Mapping Context

Ensure your input mapping has:
- **Shift**: Dash action
- **Space**: Jump action
- **Left Mouse**: Slash (AbilitySlot1)

### 3. HUD Blueprint

If you have a HUD Blueprint:
- Remove any references to heat bar or heat display
- Remove any Forge ability icons
- Focus on Stamina and WP display only

### 4. Resource Display

The game now uses only two resources:
- **Stamina**: Consumed by abilities, regenerates over time
- **Will Power (WP)**: Builds up as you use Hacker abilities (corruption meter)

### 5. Testing Checklist

After setup, test:
1. [ ] Basic slash attack works
2. [ ] Dash ability works
3. [ ] Jump ability works
4. [ ] Dash + Slash triggers Phantom Strike
5. [ ] Jump + Slash triggers Aerial Rave
6. [ ] Time slow effect works correctly
7. [ ] No crashes when restarting PIE
8. [ ] Abilities reset properly between plays

### 6. Common Issues

**If combos don't trigger:**
- Check input timing (default 0.5s window)
- Verify combo components are attached to player
- Check console for any error messages

**If time slow lasts too long:**
- Reduce TimeSlowDuration parameter
- Note: Duration is now in real-time seconds

**If abilities don't work:**
- Verify stamina/WP costs aren't too high
- Check if player has enough resources
- Ensure ability components are enabled

### 7. Optimization Tips

- Start with default values and adjust based on playtesting
- Lower TimeSlowScale = slower motion (more dramatic)
- Higher DamageMultiplier = more reward for combos
- Adjust ComboWindowTime for difficulty (shorter = harder)

## Next Steps

Once basic setup is complete:
1. Add visual effects to combo executions
2. Add sound effects for impact
3. Create UI for combo notifications
4. Balance resource costs
5. Add particle effects for abilities

Remember: All parameters are hot-reloadable in the editor, so you can tweak values while playing!