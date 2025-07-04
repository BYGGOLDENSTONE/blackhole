# Blackhole - Game Design Document
**Version**: 2.0  
**Engine**: Unreal Engine 5.5  
**Date**: 2025-07-04

## Vision & Core Pillars

**Player Role**: A hybrid hacker-forge warrior navigating dangerous environments through close combat and environmental manipulation.

**Core Pillars**:
1. **Dual Resource Management**: Balance Willpower (WP) and Heat with Stamina for tactical ability usage
2. **Path-Based Combat**: Choose between Hacker and Forge paths before each level, each with distinct ability sets
3. **Environmental Interaction**: Hack and forge objects to create tactical advantages
4. **Adaptive Gameplay**: Dynamic ability loss creates escalating risk/reward scenarios

## Resource System

### Primary Resources
- **Stamina**: Universal resource consumed by all abilities
- **Willpower (WP)**: Corruption system - starts at 0% (good), increases with Hacker abilities and enemy attacks
- **Heat**: Consumed by Forge path abilities, causes overheat lockout at 100%

### Resource Mechanics
- **Hacker Path**: Abilities consume Stamina + ADD WP (corruption cost)
- **Forge Path**: Abilities consume Stamina + Heat
- **WP Corruption**: Higher WP = More ability loss (0% WP = all abilities available)
- **Hacker Enemies**: Mindmeld ability continuously adds WP corruption to player
- **Heat System**: Only active in Forge path
- **Combo Rewards**: Ability chains reduce WP corruption or reduce Heat
- **Environmental Pickups**: WP cleansing stations and Heat vents for resource management

## Path System

### Hacker Path
**Theme**: Speed, precision, and technical mastery  
**Visual**: Cyan/blue color scheme  
**Resource**: Stamina + WP consumption

**Abilities**:
1. **Katana Slash** (LMB): 10 Stamina + 15 WP corruption - Quick precision strike with bleed effect
2. **Firewall Breach** (RMB): 15 Stamina + 20 WP corruption - Armor shred over time
3. **Pulse Hack** (Q): 5 Stamina + 10 WP corruption - Area slow with WP cleansing (reduces 5 WP per enemy)
4. **Gravity Pull** (E): 10 Stamina + 15 WP corruption - Pull objects and enemies

**Utility Abilities**:
- **Hacker Dash** (Shift): Stamina only - Fast repositioning, no damage
- **Hacker Jump** (Space): Stamina only - Enhanced air control, double jump

### Forge Path
**Theme**: Power, impact, and destructive force  
**Visual**: Red/orange color scheme  
**Resource**: Stamina + Heat consumption

**Abilities**:
1. **Forge Slam** (LMB): 15 Stamina + 20 Heat - Heavy destructive strike (planned)
2. **Molten Mace Slash** (RMB): 20 Stamina + 30 Heat - Heavy damage with burn DoT
3. **Heat Shield** (Q): 15 Stamina + 20 Heat - Damage absorption shield
4. **Blast Charge** (E): 20 Stamina + 25 Heat - Explosive knockback attack
5. **Hammer Strike** (R): 15 Stamina + 20 Heat - Stunning melee combo

**Utility Abilities**:
- **Forge Dash** (Shift): 5 Stamina only - Damaging charge with stagger
- **Forge Jump** (Space): 10 Stamina only - Ground slam with area damage

## Ability Loss & Buff System

### WP Corruption Thresholds
- **0-10% WP**: All abilities available (good state)
- **10-30% WP**: Disable 1 random ability (system strain)
- **30-60% WP**: Disable 2 random abilities (system overload)
- **>60% WP**: Disable 3 abilities (critical corruption)

### Survivor Buffs
When abilities are lost, remaining abilities gain:
- **+25% damage** per lost ability
- **-15% cooldown** per lost ability
- **-5 WP corruption** on next cast (reduces corruption)

### Rules
- One-time trigger per combat encounter
- Permanent loss until combat ends
- Utility abilities (Dash/Jump) never disabled

## Combo System

### Chain Mechanics
- **Chain Window**: 2 seconds between distinct abilities
- **Mini Combo** (3+ abilities): +2 WP or -3 Heat reward
- **Combo Surge** (5+ abilities): +10 WP + 50% next ability damage

### Requirements
- Must use distinct abilities (no repeating same ability)
- Combo resets if window expires
- Rewards scale with combo length

## Input Mapping

### Core Controls
- **WASD**: Movement
- **Mouse**: Look around
- **Tab**: Switch between Hacker/Forge paths (Development only - path is chosen pre-level)
- **H**: Toggle first/third person camera

### Path-Specific Basic Attacks
- **LMB**: Katana Slash (Hacker) / Forge Slam (Forge - planned)
- **K**: Kill (debug ability)

### Path-Specific Slots
- **LMB**: Katana Slash (Hacker) / Forge Slam (Forge - planned)
- **RMB**: Firewall Breach (Hacker) / Molten Mace (Forge)
- **Q**: Pulse Hack (Hacker) / Heat Shield (Forge)
- **E**: Gravity Pull (Hacker) / Blast Charge (Forge)
- **R**: Reserved (Hacker) / Hammer Strike (Forge)
- **F**: Reserved for ultimate abilities

### Utility Abilities
- **Shift**: Hacker Dash / Forge Dash
- **Space**: Hacker Jump / Forge Jump

## Technical Implementation

### Architecture
- **Engine**: Unreal Engine 5.5 with C++ core systems
- **No GAS Dependency**: Custom ability system for full control
- **Component-Based**: Modular ability and attribute components
- **Event-Driven**: Delegates for UI updates and system communication

### Key Systems
- **ResourceManager**: Centralized WP/Heat/Stamina management
- **ThresholdManager**: WP percentage monitoring and ability loss
- **ComboTracker**: Chain detection and reward calculation
- **Custom Ability System**: Polymorphic ability components

### Performance Considerations
- Event-driven updates over polling
- Object pooling for VFX and projectiles
- Cached component references
- Minimal Blueprint-C++ transitions

## Enemy Design

### Enemy Types
- **Hacker Enemies**: Use Mindmeld ability to continuously corrupt player's WP
- **Forge Enemies**: Focus on direct damage and area control
- **Hybrid Enemies**: Combine multiple threat types
- **Environmental Integration**: Enemies interact with hackable/forgeable objects

### Hacker Enemy - Mindmeld Ability
- **Range**: 50 meter maximum range
- **Line of Sight**: Requires clear visual connection to player
- **Corruption Rate**: 1.0 WP corruption per second (configurable)
- **Visual Feedback**: Purple debug line shows active connection
- **Counterplay**: Break line of sight or eliminate enemy to stop corruption

### AI Behaviors
- **Adaptive Tactics**: Enemies respond to player's current path
- **Environmental Usage**: AI uses hackable objects and cover
- **Threat Assessment**: Prioritize based on player's active abilities
- **Hacker Enemy Strategy**: Maintain line of sight for continuous WP corruption

## Progression System

### Onboarding Flow
1. **Basic Combat**: Slash and movement tutorial
2. **Path Selection**: Choose between Hacker or Forge before entering level
3. **Path Mastery**: Master your chosen path's abilities and resource management
4. **Combo Mastery**: Chain abilities for maximum effectiveness
5. **Resource Crisis**: Handle ability loss scenarios

### Skill Expression
- **Path Selection**: Choose the right path for each level's challenges
- **Resource Management**: Balance aggressive plays with conservation
- **Combo Optimization**: Maximize chain rewards and damage
- **Crisis Adaptation**: Effective play when abilities are disabled

## Implementation Status

### ✅ Completed Systems
- Dual resource consumption (Stamina + WP/Heat)
- Path-based ability switching
- All 11 core abilities implemented
- Threshold manager and ability loss
- Combo tracking and rewards
- Complete HUD with path-appropriate resource display
- Console commands for testing

### 🔄 Current Focus
- Resource balance tuning
- VFX and audio integration
- Environment interaction polish

### 🎯 Future Enhancements
- Ultimate abilities (F key)
- Environmental destruction
- Advanced enemy AI
- Narrative integration