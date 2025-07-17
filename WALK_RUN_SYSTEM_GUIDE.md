# Walk/Run System Guide

## Overview
The player character now has a walk/run toggle system controlled by the W key (forward movement).

## How It Works

### Walking Mode (Default)
- **Speed**: 300 units/second
- **Activation**: Press W once to move forward
- Player starts in walk mode by default

### Running Mode
- **Speed**: 600 units/second
- **Activation**: Double-tap W within 1 second
- **Toggle**: Double-tap again to return to walking

## Key Features

### Double-Tap Detection
- **Window**: 1.0 second (configurable)
- Only forward movement (W key) triggers the double-tap
- Side/backward movement doesn't affect run state

### Speed Toggle
- Running state persists until toggled off
- Speed changes immediately on toggle
- Works with all movement directions once activated

## Configuration Settings

In `BlackholePlayerCharacter` Blueprint or C++:
- **Walk Speed**: 300.0f (default)
- **Run Speed**: 600.0f (default)
- **Double Tap Window**: 1.0f seconds
- **bIsRunning**: Read-only state indicator

## Implementation Details

### Code Structure
```cpp
// Properties added to BlackholePlayerCharacter
float WalkSpeed = 300.0f;
float RunSpeed = 600.0f;
float DoubleTapWindow = 1.0f;
bool bIsRunning = false;

// Functions
void HandleWKeyPress();      // Detects double-tap
void HandleWKeyRelease();    // Tracks key release for proper double-tap
void SetMovementSpeed(bool); // Changes movement speed
void ResetDoubleTapState();  // Cleans up double-tap detection state
```

### Double-Tap Detection (Simplified)
- Simple time-based detection between W key presses
- No complex state machine required
- 0.1 second minimum time between presses to prevent re-triggers
- Resets timing after successful double-tap to prevent triple-tap

### Debug Output
Console logs show:
- "Double tap detected! Running: ON/OFF"
- "Movement speed set to: 300/600"

## Tips for Use

1. **Quick Toggle**: Double-tap W rapidly to switch between walk/run
2. **Persistent State**: Running mode stays active across all movements
3. **Combat**: Use walk for precise positioning, run for quick escapes
4. **Customization**: Adjust speeds in Blueprint for different gameplay feel

## Future Enhancements
- Visual indicator for current movement mode
- Stamina system for limited running
- Sound effects for footsteps (walk vs run)
- Animation blueprint integration for run animations