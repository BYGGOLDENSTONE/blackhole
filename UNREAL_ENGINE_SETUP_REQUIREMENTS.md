# Unreal Engine Setup Requirements for UI System

## Overview
This document outlines all the Unreal Engine setup requirements based on the C++ UI system implementation.

## 1. Level Names Required

The code expects these specific level names:

### Main Levels
- **`MainMenu`** - Main menu level (loaded by GameOverScreen.cpp line 308)
- **`ThirdPersonMap`** - Main gameplay level (loaded by MainMenuScreen.cpp line 223)

### Level Detection
The GameStateManager checks for level names to determine initial state:
- If level contains "MainMenu", it starts in MainMenu state
- Otherwise, it starts in Playing state directly

## 2. Input Mappings Required

### Action Mappings
1. **`Pause`** - Bound to ESC key
   - Used in BlackholeHUD.cpp line 955
   - Triggers pause menu when in Playing state
   - Resumes game when in Paused state

### Enhanced Input System
The game uses Unreal's Enhanced Input system based on includes in BlackholePlayerCharacter.cpp

## 3. Game Instance Setup

### Required Game Instance Subsystems
The following subsystems must be registered with the Game Instance:

1. **UGameStateManager** (GameInstanceSubsystem)
   - Manages overall game state (MainMenu, Playing, Paused, GameOver)
   - Handles level transitions and game flow

2. **UResourceManager** (GameInstanceSubsystem)
   - Manages WillPower and Heat resources
   - Handles resource regeneration and consumption

3. **UUIManager** (GameInstanceSubsystem)
   - Manages all UI screens and transitions
   - Handles showing/hiding of menus

### World Subsystems
1. **UThresholdManager** (WorldSubsystem)
   - Manages combat state and ability disabling
   - Handles ultimate mode activation

## 4. UI Blueprint Requirements

While the C++ creates UI programmatically, for production you should create Blueprint versions:

### Widget Blueprints Needed
1. **WBP_MainMenu** (based on UMainMenuWidget)
2. **WBP_PauseMenu** (based on UPauseMenuWidget)
3. **WBP_GameOver** (based on UGameOverWidget)
4. **WBP_SimplePauseMenu** (based on USimplePauseMenu)

### HUD Blueprint
- Create a Blueprint based on ABlackholeHUD
- This will be set as the default HUD class in the GameMode

## 5. Game Mode Setup

### Game Mode Requirements
- Set default HUD class to your ABlackholeHUD blueprint
- Set default pawn class to ABlackholePlayerCharacter blueprint

## 6. Project Settings

### Input Settings
1. Add Action Mapping:
   - Action Name: "Pause"
   - Key: Escape

### Maps & Modes
1. Set Default GameMode to your custom GameMode blueprint
2. Set Editor Startup Map to either:
   - MainMenu (for production)
   - ThirdPersonMap (for testing, as the code skips menu in prototype mode)
3. Set Game Default Map to MainMenu

## 7. Level Setup

### MainMenu Level
- Empty level with just lighting
- GameStateManager will spawn UI

### ThirdPersonMap Level
- Main gameplay level
- Include player start
- Set up world settings to use your GameMode

## 8. C++ Class Dependencies

Ensure these classes are compiled and available:
- ABlackholeHUD
- UMainMenuWidget
- UPauseMenuWidget
- UGameOverWidget
- USimplePauseMenu
- UGameStateManager
- UResourceManager
- UUIManager
- UThresholdManager

## 9. Current Prototype Behavior

The current code includes prototype shortcuts:
- BlackholeHUD.cpp line 94-98: Skips main menu and starts directly in Playing state
- SimplePauseMenu is created programmatically without needing a Blueprint

For production, you should:
1. Remove the prototype code that skips the main menu
2. Create proper Blueprint versions of all widgets
3. Set up proper level transitions

## 10. Menu Controls Summary

### Main Menu
- START GAME → Loads ThirdPersonMap
- OPTIONS → Not implemented
- QUIT → Exits game

### Pause Menu (ESC during gameplay)
- RESUME → Returns to game
- RESTART → Reloads current level
- QUIT → Exits game

### Game Over Screen
- RETRY → Restarts game via GameStateManager
- MAIN MENU → Loads MainMenu level
- QUIT → Exits game

## 11. Important Notes

1. **Level Name Stripping**: The code removes PIE prefix ("UEDPIE_0_") when checking level names in editor

2. **Input Mode Management**: The UI system automatically switches between:
   - FInputModeGameOnly (during gameplay)
   - FInputModeUIOnly (when menus are shown)

3. **Mouse Cursor**: Automatically shown/hidden based on UI state

4. **Crash Safety**: The pause menu includes crash checkpoints for debugging restart functionality

## Setup Checklist

- [ ] Create MainMenu level
- [ ] Create ThirdPersonMap level (or rename existing gameplay level)
- [ ] Add "Pause" action mapping to ESC key
- [ ] Create GameMode blueprint with ABlackholeHUD as HUD class
- [ ] Set up Game Instance class if using custom one
- [ ] Configure default maps in Project Settings
- [ ] (Optional) Create Blueprint versions of UI widgets
- [ ] Test level transitions and pause functionality