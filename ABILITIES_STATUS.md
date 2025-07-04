# Abilities Implementation Status
**Engine**: Unreal Engine 5.5  
**Date**: 2025-07-04

## Overview
All abilities now use the **dual resource system**:
- **Hacker Path**: Stamina + WP corruption (WP increases, which is bad)
- **Forge Path**: Stamina + Heat consumption (combat abilities only)
- **WP Corruption System**: 0% WP = all abilities, higher WP = fewer abilities
- **Hacker Enemies**: Mindmeld ability continuously adds WP corruption to player
- **Path Selection**: Choose your path before entering each level (no mid-combat switching)

## ✅ Implemented Abilities (11 total + 1 planned)

### Basic Abilities (Path-Specific)

**Katana Slash** (Hacker) - LMB
- **Cost**: 10 Stamina + 15 WP corruption
- **Cooldown**: 2 seconds
- **Damage**: 20 (with bleed effect)
- **Range**: 200 units
- **Description**: Quick precision strike that inflicts bleeding damage over time. Primary melee attack for Hacker path.

**Forge Slam** (Forge) - LMB  
- **Status**: ⚠️ Planned
- **Cost**: 15 Stamina + 20 Heat
- **Cooldown**: 2 seconds
- **Description**: Heavy destructive ground strike. Primary melee attack for Forge path.

**Kill** (Debug) - K
- **Cost**: None (Debug ability)
- **Cooldown**: 5 seconds
- **Range**: 500 units
- **Description**: Instantly destroys target actor. Development testing tool available in both paths.

### Hacker Path Abilities

**Pulse Hack** - Q
- **Cost**: 5 Stamina + 10 WP corruption
- **Cooldown**: 8 seconds
- **Radius**: 500 units
- **Duration**: 3 seconds slow effect
- **Special**: Cleanses 5 WP per affected enemy
- **Description**: Releases a digital pulse that slows all enemies in radius by 50%. Each affected enemy reduces your WP corruption, making this a key recovery tool.

**Firewall Breach** - RMB
- **Cost**: 15 Stamina + 20 WP corruption
- **Cooldown**: 4 seconds
- **Duration**: 5 seconds
- **Armor Reduction**: 30%
- **Range**: 300 units
- **Description**: Corrupts target's defenses, reducing their armor over time. Allows subsequent attacks to deal increased damage.

**Gravity Pull** - E
- **Cost**: 10 Stamina + 15 WP corruption  
- **Cooldown**: 3 seconds
- **Range**: 2000 units (20 meters)
- **Force**: 50000 impulse
- **Description**: Launches a gravitational anomaly that pulls enemies and objects toward you. Can affect multiple targets and pull hackable objects.

### Forge Path Abilities

**Molten Mace Slash** - RMB
- **Cost**: 20 Stamina + 30 Heat
- **Cooldown**: 5 seconds
- **Damage**: 40 base + 10 burn/sec for 3 seconds
- **Range**: 300 units cone (60° angle)
- **Description**: Devastating cone attack that deals heavy damage and applies burning DoT. Hits all enemies in front of you.

**Heat Shield** - Q  
- **Cost**: 15 Stamina + 20 Heat
- **Cooldown**: 12 seconds
- **Shield Health**: 100
- **Duration**: 8 seconds or until depleted
- **Description**: Generates a protective barrier that absorbs incoming damage. Shield visually cracks as it takes damage.

**Blast Charge** - E
- **Cost**: 20 Stamina + 25 Heat
- **Cooldown**: 10 seconds
- **Damage**: 30 explosion damage
- **Radius**: 400 units
- **Knockback**: 800 units impulse
- **Description**: Explosive charge that damages and knocks back all nearby enemies. Great for crowd control.

**Hammer Strike** - R
- **Cost**: 15 Stamina + 20 Heat
- **Cooldown**: 6 seconds  
- **Damage**: 25 per hit (3 hits max)
- **Stun Duration**: 1.5 seconds
- **Combo Window**: 1 second between hits
- **Description**: Stunning melee combo that can chain up to 3 hits. First hit stuns, subsequent hits deal increased damage.

### Utility Abilities (Path Variants)

**Hacker Dash** (Hacker) - Shift
- **Cost**: 5 Stamina only
- **Cooldown**: 1 second
- **Distance**: 1000 units
- **Duration**: 0.2 seconds
- **Description**: Lightning-fast repositioning dash. No damage but provides brief invincibility frames. Perfect for dodging attacks.

**Forge Dash** (Forge) - Shift  
- **Cost**: 5 Stamina only
- **Cooldown**: 2 seconds
- **Distance**: 800 units
- **Duration**: 0.3 seconds
- **Impact Damage**: 10
- **Stagger Duration**: 0.5 seconds
- **Description**: Charging dash that damages and staggers enemies on impact. Slower than Hacker dash but offensive.

**Hacker Jump** (Hacker) - Space
- **Cost**: 10 Stamina only
- **Cooldown**: None
- **Jump Height**: 600 units
- **Special**: Double jump capability
- **Description**: Enhanced vertical mobility with double jump. Provides excellent air control for platforming.

**Forge Jump** (Forge) - Space
- **Cost**: 10 Stamina only  
- **Cooldown**: 3 seconds
- **Jump Height**: 800 units
- **Slam Damage**: 20
- **Slam Radius**: 300 units
- **Min Height for Slam**: 200 units
- **Description**: Powerful jump that creates damaging shockwave on landing from sufficient height.

## ✅ Enemy Abilities

**Mindmeld** (Hacker Enemy)
- **Trigger**: Line of sight to player
- **Range**: 5000 units (50 meters)
- **Effect**: +1.0 WP corruption per second
- **Visual**: Purple line connects enemy to player
- **Counterplay**: Break line of sight or eliminate enemy
- **Description**: Continuous corruption beam that increases player's WP as long as connection is maintained. Primary threat from Hacker enemies.

**Dodge Component** (Agile Enemies)
- **Dodge Chance**: 30%
- **Cooldown**: 2 seconds between dodges
- **Description**: Allows enemies to evade incoming attacks with quick sidesteps.

**Block Component** (Tank Enemies)  
- **Damage Reduction**: 50% when blocking
- **Block Duration**: 2 seconds
- **Block Cooldown**: 4 seconds
- **Description**: Defensive stance that significantly reduces incoming damage.

**Smash Ability** (Heavy Enemies)
- **Damage**: 40
- **Range**: 150 units
- **Stun Duration**: 1 second
- **Description**: Powerful ground pound attack with area damage and brief stun.

## Future Development
- **F Key**: Reserved for ultimate abilities (both paths)
- **Additional Hacker abilities** for R key slot
- **Environmental interaction abilities** for hackable/forgeable objects

## Input Mapping

### Path-Based Ability Slots
Same keys activate different abilities based on path chosen before level:

| Key | Hacker Path | Forge Path |
|-----|-------------|------------|
| **LMB** | Katana Slash | Forge Slam (⚠️ planned) |
| **RMB** | Firewall Breach | Molten Mace |
| **Q** | Pulse Hack | Heat Shield |
| **E** | Gravity Pull | Blast Charge |
| **R** | (Empty) | Hammer Strike |
| **F** | (Reserved) | (Reserved) |

### Shared/Special Keys
- **K**: Kill (Debug - both paths)
- **Shift**: Path-based dash
- **Space**: Path-based jump
- **Tab**: Switch paths (Development only)
- **H**: Camera toggle


## Resource System

### Stamina (Universal)
- **Max**: 100
- **Regen Rate**: 10/second
- **Primary Consumer**: Slash attacks and abilities
- **Depletion**: Prevents all ability usage

### Willpower (Hacker Path)
- **Start**: 0% (good state)
- **Max**: 100% (corrupted state)
- **Sources**: Hacker abilities, Mindmeld enemies
- **Reduction**: Combo rewards, Pulse Hack cleansing
- **Effect**: Higher WP = fewer available abilities

### Heat (Forge Path)
- **Start**: 0
- **Max**: 100 (overheat lockout)
- **Dissipation**: 5/second
- **Sources**: Forge combat abilities only
- **Effect**: 100% = temporary ability lockout

## Combat Mechanics

### Combo System
- **Window**: 2 seconds between abilities
- **Mini Combo** (3+ different abilities): Reduces WP by 2 or Heat by 3
- **Combo Surge** (5+ different abilities): Reduces WP by 10 + 50% damage on next ability
- **Requirement**: Must use different abilities (no repeats)

### WP Corruption Thresholds
- **0-10%**: All abilities available
- **10-30%**: 1 random ability disabled
- **30-60%**: 2 random abilities disabled  
- **60%+**: 3 abilities disabled

### Survivor Buffs (per disabled ability)
- **+25% damage** to remaining abilities
- **-15% cooldown** reduction
- **-5 WP** on next ability cast

## Testing
Use console commands for testing:
- `SetWP <amount>` - Set WillPower
- `SetHeat <amount>` - Set Heat  
- `SetPath <Hacker|Forge>` - Switch paths (Development only)
- `ShowDebugInfo` - Display resource status
- `StartCombat` / `EndCombat` - Test combat systems

## Summary
- **11 abilities** fully implemented with dual resource system
- **Path-based organization** complete
- **Resource consumption** working correctly
- **Ready for testing** and balance tuning