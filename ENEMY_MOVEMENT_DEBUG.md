# Enemy Movement and Rotation Debug Guide

## Issues Identified
1. Enemies' state machines are working but they're not moving or rotating
2. HackerEnemy gets stuck in Channeling state
3. Movement commands (MoveTo) may be called but no actual movement occurs

## Debug Logging Added

### 1. **Chase State Movement Debug** (`ChaseState.cpp`)
- Added detailed logging for MoveTo calls including target location and acceptance radius
- Added MoveTo result logging (Success/Failed/AlreadyAtGoal)
- Added NavMesh validation when movement fails
- Added velocity check to detect if enemy is actually moving

### 2. **Rotation Debug** (`EnemyStateBase.cpp`)
- Added rotation logging showing current, desired, and new rotation values
- Only logs when meaningful rotation is needed (>1 degree difference)

### 3. **AIController Debug** (`BlackholeAIController.cpp`)
- Added movement configuration logging
- Added check to ensure movement mode is set to Walking
- Logs max walk speed and movement mode

### 4. **BaseEnemy Initialization Debug** (`BaseEnemy.cpp`)
- Added logging to verify CharacterMovement component exists
- Added logging to verify AIController is properly possessed
- Logs default walk speed and AutoPossessAI setting

### 5. **HackerEnemy Channeling Fix**
- Reduced MaxChannelTime from 5.0f to 2.0f seconds to prevent getting stuck
- Already has proper exit conditions for line of sight loss and distance

## Common Issues to Check in Editor

### 1. **NavMesh Issues**
- **Check**: Is there a NavMeshBoundsVolume in the level?
- **Check**: Is the NavMesh properly built? (Press P in editor to visualize)
- **Fix**: Add NavMeshBoundsVolume covering play area and rebuild paths

### 2. **Enemy Blueprint Configuration**
- **Check**: Is the enemy Blueprint's movement component properly configured?
- **Check**: Default walk speed should be > 0 (e.g., 300-600)
- **Check**: Is "Use Controller Rotation Yaw" unchecked?
- **Check**: Is "Orient Rotation to Movement" unchecked?

### 3. **AIController Assignment**
- **Check**: Is the AIController class set in the enemy Blueprint?
- **Check**: Auto Possess AI should be "Placed in World or Spawned"
- **Fix**: In enemy Blueprint, set AI Controller Class to "BlackholeAIController"

### 4. **Movement Component**
- **Check**: Does the enemy have a CharacterMovementComponent?
- **Check**: Is Max Walk Speed > 0?
- **Check**: Movement Mode should be "Walking"

### 5. **State Machine Initialization**
- The logs will show if state machine is properly initialized
- Should see "State machine initialization complete - tick enabled"

## Expected Log Output When Working

```
BaseEnemy_1: Default walk speed set to 400
BaseEnemy_1: AIController present: BlackholeAIController_1
BaseEnemy_1 StateMachine: BeginPlay - Owner set
BaseEnemy_1: Found player target in BeginPlay
HackerEnemyStateMachine_1: Initializing state machine
HackerEnemyStateMachine_1: 6 states registered
HackerEnemyStateMachine_1: Target is set: BlackholePlayerCharacter_1
BaseEnemy_1: Entered Idle state
BaseEnemy_1: MoveTo called - Target: Player, Location: X=100 Y=200 Z=0, AcceptRadius: 960
BaseEnemy_1: MoveTo result: Success
```

## Troubleshooting Steps

1. **Run the game and check the log output**
   - Look for ERROR messages first
   - Check if AIController is possessed
   - Check if MoveTo is returning Success or Failed

2. **If MoveTo fails:**
   - Check NavMesh (press P in editor)
   - Check if "No navigation system found!" appears in logs
   - Verify level has NavMeshBoundsVolume

3. **If MoveTo succeeds but no movement:**
   - Check velocity warnings in logs
   - Verify movement component settings in Blueprint
   - Check if movement is disabled somewhere

4. **If rotation isn't working:**
   - Check rotation logs (set Log verbosity to VeryVerbose)
   - Verify SetActorRotation is being called
   - Check if something else is overriding rotation

5. **For HackerEnemy channeling issue:**
   - Should now exit after 2 seconds
   - Check if line of sight is maintained
   - Verify MindmeldComponent exists on HackerEnemy

## Next Steps

After running with these debug logs:
1. Check which specific errors appear
2. Focus on the first error in the chain
3. Most likely issues are:
   - Missing NavMesh in level
   - Blueprint configuration issues
   - Movement component settings

The debug logging will pinpoint exactly where the problem is occurring.