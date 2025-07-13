# Enemy Stats Data Table Guide

## Overview
The enemy stats data table system allows you to configure all enemy properties in a single location within Unreal Engine. This makes balancing and tweaking enemy stats much easier without recompiling code.

## Setup Instructions

### 1. Create the Data Table Asset
1. In Unreal Editor, go to Content Browser
2. Right-click → Miscellaneous → Data Table
3. Choose "FEnemyStatsData" as the Row Structure
4. Name it "DT_EnemyStats" (or any name you prefer)

### 2. Import Default Data (Optional)
1. Use the provided `EnemyStatsDataTable_Template.csv` file
2. Right-click your data table → Reimport
3. Select the CSV file
4. This will populate the table with default values

### 3. Configure Enemy Blueprints
1. Open each enemy Blueprint (BP_TankEnemy, BP_AgileEnemy, etc.)
2. In the Details panel, find the "Stats" category
3. Set "Enemy Stats Data Table" to your created data table
4. The "Stats Row Name" should already be set (Tank, Agile, Combat, or Hacker)

### 4. Edit Stats in Data Table
1. Double-click your data table asset
2. You'll see all enemy types in rows
3. Click any cell to edit values
4. Changes apply immediately when you play

## Property Categories

### Basic Stats
- **EnemyName**: Display name for the enemy
- **EnemyType**: Type identifier (Tank, Agile, Combat, Hacker)
- **MaxHealth**: Starting health points

### Movement
- **MovementSpeed**: Base movement speed (300-600 recommended)
- **Acceleration**: How quickly enemy reaches max speed
- **BrakingDeceleration**: How quickly enemy stops
- **RotationRate**: Turn speed in degrees/second
- **Mass**: Weight affects knockback resistance

### Combat Ranges
- **AttackRange**: Distance to start attacking
- **ChaseRange**: Distance to start chasing
- **SightRange**: Distance to detect player
- **PreferredCombatDistance**: Ideal fighting distance
- **MinimumEngagementDistance**: Closest approach distance

### Damage & Attack Speed
- **BaseDamage**: Base melee damage
- **AttackSpeedMultiplier**: Multiplies attack animation speed
- **AttackCooldown**: Time between attacks

### AI Behavior
- **RetreatHealthPercent**: Health % to start retreating (0 = never)
- **DefensiveHealthPercent**: Health % to become defensive
- **AggressionLevel**: 0-1, affects decision making
- **ReactionTime**: Delay before responding to threats
- **SearchDuration**: How long to search for lost player
- **MaxTimeInCombat**: Max continuous combat time
- **PatrolWaitTime**: Pause duration during patrol

### Defensive Abilities
- **DodgeChance**: 0-1 chance to dodge attacks
- **BlockChance**: 0-1 chance to block attacks
- **ReactiveDefenseChance**: 0-1 chance to react defensively
- **DefensiveCooldown**: Time between defensive actions

### Special Abilities

#### Dash (Agile Enemy)
- **bCanDash**: Enable dash ability
- **DashCooldown**: Time between dashes
- **DashBehindDistance**: Distance behind target after dash
- **DashForce**: Impulse strength for dash

#### Area Damage (Tank Enemy)
- **bHasAreaDamage**: Enable ground slam
- **GroundSlamRadius**: Area effect radius
- **GroundSlamDamageMultiplier**: Damage multiplier for slam
- **GroundSlamKnockbackForce**: Push force on hit
- **GroundSlamCooldown**: Time between slams
- **GroundSlamStaggerTime**: Tank vulnerability duration after slam

#### Ranged Attack (Hacker Enemy)
- **bHasRangedAttack**: Enable ranged abilities
- **RangedAttackRange**: Maximum attack distance
- **RangedAttackDamage**: Projectile damage
- **RangedAttackCooldown**: Time between shots
- **SafeDistance**: Preferred distance from player

#### Melee Attack
- **bHasSmashAbility**: Enable melee attacks
- **SmashDamage**: Melee attack damage
- **SmashCooldown**: Time between melee attacks
- **SmashRange**: Melee attack reach
- **SmashKnockbackForce**: Push force on hit

#### Block Ability
- **bCanBlock**: Enable blocking
- **BlockDuration**: How long block lasts
- **BlockCooldown**: Time between blocks
- **BlockDamageReduction**: % damage reduced when blocking

#### Dodge Ability
- **bCanDodge**: Enable dodging
- **DodgeCooldown**: Time between dodges
- **DodgeDistance**: Distance covered by dodge
- **DodgeImpulse**: Force applied during dodge

## Tips for Balancing

### Tank Enemy
- High health (150+), slow speed (300)
- Strong area attacks with knockback
- Can block but can't dodge
- High aggression, never retreats

### Agile Enemy
- Low health (80), fast speed (600)
- Frequent dashes, high dodge chance
- Short attack range (dagger-like)
- Maximum aggression, never retreats

### Combat Enemy
- Balanced stats across the board
- Can both block and dodge
- Medium aggression
- Versatile fighter

### Hacker Enemy
- Lowest health (60), ranged attacker
- Maintains safe distance
- Can block and dodge
- Lower aggression, tactical

## Testing Changes

1. Make changes in the data table
2. Save the data table (Ctrl+S)
3. Play in Editor - changes apply immediately
4. No need to recompile code!

## Exporting/Importing

### Export to CSV
1. Right-click data table → Export
2. Save as CSV
3. Edit in Excel/Google Sheets
4. Useful for bulk changes

### Import from CSV
1. Right-click data table → Reimport
2. Select your edited CSV
3. All values update at once

## Troubleshooting

### Stats Not Loading
- Check enemy Blueprint has data table assigned
- Verify row name matches (case-sensitive)
- Check output log for error messages

### Values Not Updating
- Make sure to save the data table
- Some values only apply on enemy spawn
- Try restarting PIE session

### CSV Import Issues
- Column headers must match exactly
- Use TRUE/FALSE for boolean values
- Numbers should not have commas
- Text values can have spaces

## Advanced Usage

### Creating Variants
1. Duplicate a row in the data table
2. Give it a new name (e.g., "TankElite")
3. Modify stats as desired
4. Create enemy variant Blueprint
5. Set its row name to "TankElite"

### Runtime Stat Changes
```cpp
// In C++ or Blueprint
FEnemyStatsData NewStats = UEnemyStatsManager::GetEnemyStats(
    this, 
    MyDataTable, 
    FName("TankElite")
);
UEnemyStatsManager::ApplyStatsToEnemy(Enemy, NewStats);
```

This system provides complete control over enemy balancing without touching code!