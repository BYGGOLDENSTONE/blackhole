# Blackhole Project - Blueprint Setup Guide
**Engine**: Unreal Engine 5.5  
**Date**: 2025-07-04

## Quick Setup Checklist

### 1. Input System
Create Input Actions in `Content/Input/Actions/`:
- **IA_Move** (Vector2D) - WASD movement
- **IA_Look** (Vector2D) - Mouse look
- **IA_Jump** (Digital) - Space key
- **IA_CameraToggle** (Digital) - H key
- **IA_SwitchPath** (Digital) - Tab key (Development only)
- **IA_Dash** (Digital) - Shift key
- **IA_UtilityJump** (Digital) - Space key
- **IA_AbilitySlot1** (Digital) - Left Mouse Button
- **IA_AbilitySlot2** (Digital) - Right Mouse Button
- **IA_AbilitySlot3** (Digital) - Q key
- **IA_AbilitySlot4** (Digital) - E key
- **IA_AbilitySlot5** (Digital) - R key
- **IA_AbilitySlot6** (Digital) - F key
- **IA_Kill** (Digital) - K key (debug)

Create **IMC_Default** Input Mapping Context with all actions.

### 2. Player Character Blueprint
Create **BP_BlackholePlayerCharacter** based on `ABlackholePlayerCharacter`.

**Components Setup**:
```
├── IntegrityComponent (Max: 100)
├── StaminaComponent (Max: 100, Regen: 10)
├── WillPowerComponent (Max: 100, Start: 0 - corruption system)
├── HeatComponent (Max: 100, Dissipation: 5)
├── SlashAbility (Stamina: 10, WP: 15)
├── KillAbility (Debug)
├── GravityPullAbility (Stamina: 10, WP: 15)
├── HackerDashAbility (Free)
├── ForgeDashAbility (Stamina: 5, Heat: 5)
├── HackerJumpAbility (Free)
├── ForgeJumpAbility (Stamina: 10, Heat: 10)
├── PulseHackAbility (Stamina: 5, WP: 10)
├── FirewallBreachAbility (Stamina: 15, WP: 20)
├── MoltenMaceSlashAbility (Stamina: 20, Heat: 30)
├── HeatShieldAbility (Stamina: 15, Heat: 20)
├── BlastChargeAbility (Stamina: 20, Heat: 25)
├── HammerStrikeAbility (Stamina: 15, Heat: 20)
└── ComboTracker
```

### 3. Game Mode Setup
Create **BP_BlackholeGameMode**:
- **DefaultPawnClass**: BP_BlackholePlayerCharacter
- **HUDClass**: BP_BlackholeHUD
- **PlayerControllerClass**: BP_BlackholePlayerController

Create **BP_BlackholePlayerController**:
- **CheatClass**: BlackholeCheatManager

### 4. HUD Configuration
Create **BP_BlackholeHUD** based on `ABlackholeHUD`:
- **Integrity**: Red bar (top)
- **Stamina**: Green bar 
- **WillPower**: Blue bar
- **Heat**: Orange bar (only visible in Forge path)
- **Path Display**: Current path with color coding

### 5. Path-Based Ability System
Player chooses a path before entering each level. Abilities are determined by selected path:

**Both Paths**:
- **LMB**: Slash (basic attack)
- **K**: Kill (debug)

**Hacker Path** (Stamina + WP Corruption):
- **RMB**: Firewall Breach
- **Q**: Pulse Hack (cleanses WP corruption from enemies)
- **E**: Gravity Pull
- **Shift**: Hacker Dash (5 Stamina)
- **Space**: Hacker Jump (10 Stamina)

**Forge Path** (Stamina + Heat for combat abilities):
- **RMB**: Molten Mace Slash
- **Q**: Heat Shield
- **E**: Blast Charge
- **R**: Hammer Strike
- **Shift**: Forge Dash (5 Stamina only)
- **Space**: Forge Jump (10 Stamina only)

### 6. Console Commands
Enable cheat manager and use these commands:
- `SetWP <amount>` - Set WillPower corruption (0 = good, 100 = bad)
- `SetHeat <amount>` - Set Heat
- `SetPath <Hacker|Forge>` - Switch path (Development only)
- `StartCombat` / `EndCombat` - Test combat systems
- `ResetResources` - Reset all resources
- `ShowDebugInfo` - Display resource status

### 7. Enemy Setup
Create enemy blueprints based on existing classes:

**BP_HackerEnemy** (based on AHackerEnemy):
- **MindmeldComponent**: DrainRate = 1.0 (WP corruption per second)
- **Range**: 5000 units (50 meters)
- **Behavior**: Automatically targets player and maintains line of sight
- **Visual**: Purple debug line shows active mindmeld connection

### 8. Testing Checklist
- [ ] All input actions work
- [ ] Path switching (Tab) changes abilities (Development only)
- [ ] Heat bar only shows in Forge path
- [ ] Resource consumption works correctly
- [ ] Camera toggle (H) works
- [ ] All abilities execute with proper costs
- [ ] Combo system tracks chains
- [ ] HUD updates in real-time
- [ ] Hacker enemies increase player WP corruption with mindmeld
- [ ] WP corruption triggers ability loss at correct thresholds

## Common Issues
1. **Abilities not working**: Check ResourceManager initialization and WP/Heat values
2. **Input not responding**: Verify Input Mapping Context assignment
3. **Heat not showing**: Ensure player is in Forge path
4. **Console commands not working**: Check CheatClass assignment in PlayerController

## File Organization
Abilities are organized by path:
- **Basic/**: SlashAbility, KillAbility
- **Hacker/**: GravityPull, PulseHack, FirewallBreach  
- **Forge/**: MoltenMace, HeatShield, BlastCharge, HammerStrike
- **Utility/**: Path-specific Dash/Jump variants