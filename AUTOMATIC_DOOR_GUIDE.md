# Automatic Door System Guide

## Overview
The AutomaticDoor is an interactive actor that opens when the player approaches AND looks at it, then closes automatically when the player passes through.

## Features
- **Proximity Detection**: Door detects when player is within range
- **Look-At Requirement**: Player must be looking at the door (configurable angle threshold)
- **Smooth Movement**: Door moves up/down smoothly with configurable speed
- **Auto-Close**: Door closes automatically when player exits the inside area
- **State Management**: Proper state machine prevents conflicting behaviors

## Door States
1. **Closed**: Default state, door is at ground level
2. **Opening**: Door is moving upward
3. **Open**: Door is fully raised
4. **Closing**: Door is moving downward

## Setup Instructions

### 1. Create Blueprint
1. Create a new Blueprint based on `AAutomaticDoor`
2. Name it `BP_AutomaticDoor`

### 2. Configure Components
- **DoorMesh**: Assign your door static mesh
- **ProximityTrigger**: Automatically sized based on ProximityRange
- **InsideTrigger**: Position this where you want to detect "player is inside"

### 3. Key Settings
- **Door Height** (300.0): How high the door moves up
- **Move Speed** (200.0): Units per second movement speed
- **Proximity Range** (400.0): Detection radius around door
- **Look At Threshold** (0.7): Dot product threshold (0.7 = ~45°, 0.5 = 60°, 0.866 = 30°)
- **Auto Close Delay** (2.0): Seconds before auto-closing (when player stops looking)

## How It Works

### Opening Conditions
Door opens when ALL conditions are met:
1. Player is within ProximityRange
2. Player is looking at the door (within angle threshold)
3. Door is currently closed

### Closing Conditions
Door closes when ANY condition is met:
1. Player exits the InsideTrigger volume (immediate close)
2. Player stops looking at door for AutoCloseDelay seconds
3. Player leaves proximity area

## Blueprint Usage Example

```cpp
// Example door settings for a secure facility door
DoorHeight = 400.0f;        // Taller door
MoveSpeed = 150.0f;         // Slower, more dramatic
ProximityRange = 600.0f;    // Larger detection area
LookAtThreshold = 0.866f;   // Require more direct look (30°)
AutoCloseDelay = 0.0f;      // No delay, security door
```

## Debug Visualization
In editor with debug enabled:
- **Yellow Box**: Player detected in proximity
- **Green Box**: Player detected AND looking at door
- **Red Line**: Player look direction (not looking at door)
- **Green Line**: Player look direction (looking at door)

## Common Issues & Solutions

### Door Not Opening
1. Check ProximityRange is large enough
2. Verify LookAtThreshold isn't too high (lower = more forgiving)
3. Ensure door mesh collision doesn't block triggers

### Door Closing Too Soon
1. Increase AutoCloseDelay
2. Make InsideTrigger volume larger
3. Check trigger overlap settings

### Door Stuck Open/Closed
1. Verify no objects blocking door movement path
2. Check state transitions in logs
3. Ensure MoveSpeed > 0

## Advanced Features (Future)
- Sound effects for open/close
- Visual indicators (lights, particles)
- Security lockdown mode
- Multiple door synchronization
- Key card requirements