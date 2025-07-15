#include "Data/EnemyStatsData.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/TankEnemy.h"
#include "Enemy/AgileEnemy.h"
#include "Enemy/CombatEnemy.h"
#include "Enemy/HackerEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Enemy/AI/EnemyStates.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/Abilities/Enemy/DodgeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DataTable.h"

FEnemyStatsData UEnemyStatsManager::GetEnemyStats(UObject* WorldContextObject, UDataTable* EnemyDataTable, FName RowName)
{
	FEnemyStatsData DefaultStats;
	
	if (!EnemyDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("GetEnemyStats: No data table provided!"));
		return DefaultStats;
	}
	
	FEnemyStatsData* StatsRow = EnemyDataTable->FindRow<FEnemyStatsData>(RowName, TEXT("GetEnemyStats"));
	
	if (!StatsRow)
	{
		UE_LOG(LogTemp, Error, TEXT("GetEnemyStats: Could not find row %s in data table"), *RowName.ToString());
		return DefaultStats;
	}
	
	return *StatsRow;
}

void UEnemyStatsManager::ApplyStatsToEnemy(ABaseEnemy* Enemy, const FEnemyStatsData& Stats)
{
	if (!Enemy)
	{
		UE_LOG(LogTemp, Error, TEXT("ApplyStatsToEnemy: Null enemy provided!"));
		return;
	}
	
	// Apply health as WP
	Enemy->SetMaxWP(Stats.MaxHealth);
	Enemy->SetCurrentWP(Stats.MaxHealth);
	
	// Apply movement stats
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = Stats.MovementSpeed;
		Movement->MaxAcceleration = Stats.Acceleration;
		Movement->BrakingDecelerationWalking = Stats.BrakingDeceleration;
		Movement->RotationRate = FRotator(0.0f, Stats.RotationRate, 0.0f);
		Movement->Mass = Stats.Mass;
	}
	
	// Apply combat range stats
	Enemy->MinimumEngagementDistance = Stats.MinimumEngagementDistance;
	
	// Apply AI parameters through state machine
	if (UEnemyStateMachine* StateMachine = Enemy->FindComponentByClass<UEnemyStateMachine>())
	{
		FEnemyAIParameters AIParams;
		
		// Health thresholds
		AIParams.RetreatHealthPercent = Stats.RetreatHealthPercent;
		AIParams.DefensiveHealthPercent = Stats.DefensiveHealthPercent;
		
		// Distance thresholds
		AIParams.AttackRange = Stats.AttackRange;
		AIParams.ChaseRange = Stats.ChaseRange;
		AIParams.SightRange = Stats.SightRange;
		AIParams.PreferredCombatDistance = Stats.PreferredCombatDistance;
		
		// Timing parameters
		AIParams.ReactionTime = Stats.ReactionTime;
		AIParams.SearchDuration = Stats.SearchDuration;
		AIParams.MaxTimeInCombat = Stats.MaxTimeInCombat;
		AIParams.PatrolWaitTime = Stats.PatrolWaitTime;
		
		// Combat parameters
		AIParams.DodgeChance = Stats.DodgeChance;
		AIParams.BlockChance = Stats.BlockChance;
		AIParams.ReactiveDefenseChance = Stats.ReactiveDefenseChance;
		AIParams.AttackCooldown = Stats.AttackCooldown / Stats.AttackSpeedMultiplier;
		AIParams.AbilityCooldown = Stats.DefensiveCooldown;
		
		// Personality
		AIParams.AggressionLevel = Stats.AggressionLevel;
		
		StateMachine->SetAIParameters(AIParams);
	}
	
	// Apply ability configurations
	if (USmashAbilityComponent* SmashAbility = Enemy->FindComponentByClass<USmashAbilityComponent>())
	{
		SmashAbility->SetDamage(Stats.SmashDamage);
		SmashAbility->SetCooldownTime(Stats.SmashCooldown / Stats.AttackSpeedMultiplier);
		SmashAbility->SetRange(Stats.SmashRange);
		SmashAbility->SetKnockbackForce(Stats.SmashKnockbackForce);
		
		// Area damage for ground slam
		if (Stats.bHasAreaDamage)
		{
			SmashAbility->SetAreaDamage(true);
			SmashAbility->SetAreaRadius(Stats.GroundSlamRadius);
		}
	}
	
	// Apply enemy-specific stats
	if (ATankEnemy* Tank = Cast<ATankEnemy>(Enemy))
	{
		Tank->GroundSlamRadius = Stats.GroundSlamRadius;
		Tank->GroundSlamDamageMultiplier = Stats.GroundSlamDamageMultiplier;
		Tank->GroundSlamKnockbackForce = Stats.GroundSlamKnockbackForce;
		Tank->AttackRange = Stats.AttackRange;
		Tank->ChaseRange = Stats.ChaseRange;
		Tank->BlockChance = Stats.BlockChance;
	}
	else if (AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy))
	{
		Agile->AttackRange = Stats.AttackRange;
		Agile->ChaseRange = Stats.ChaseRange;
		Agile->DodgeChance = Stats.DodgeChance;
		Agile->MovementSpeed = Stats.MovementSpeed;
		Agile->AttackSpeedMultiplier = Stats.AttackSpeedMultiplier;
		Agile->DashCooldown = Stats.DashCooldown;
		Agile->DashBehindDistance = Stats.DashBehindDistance;
	}
	else if (ACombatEnemy* Combat = Cast<ACombatEnemy>(Enemy))
	{
		Combat->AttackRange = Stats.AttackRange;
		Combat->ChaseRange = Stats.ChaseRange;
		Combat->DodgeChance = Stats.DodgeChance;
		Combat->BlockChance = Stats.BlockChance;
	}
	else if (AHackerEnemy* Hacker = Cast<AHackerEnemy>(Enemy))
	{
		Hacker->MindmeldRange = Stats.RangedAttackRange;
		Hacker->SafeDistance = Stats.SafeDistance;
	}
	
	// Apply block ability settings
	if (UBlockComponent* BlockAbility = Enemy->FindComponentByClass<UBlockComponent>())
	{
		BlockAbility->BlockDuration = Stats.BlockDuration;
		BlockAbility->SetCooldown(Stats.BlockCooldown);
		// Note: DamageReduction would need to be added to BlockComponent if desired
	}
	
	// Apply dodge ability settings
	if (UDodgeComponent* DodgeAbility = Enemy->FindComponentByClass<UDodgeComponent>())
	{
		DodgeAbility->SetCooldown(Stats.DodgeCooldown);
		DodgeAbility->DodgeDistance = Stats.DodgeDistance;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Applied stats from data table to enemy: %s"), *Enemy->GetName());
}