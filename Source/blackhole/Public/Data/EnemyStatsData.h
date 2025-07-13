#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyStatsData.generated.h"

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	None UMETA(DisplayName = "None"),
	Tank UMETA(DisplayName = "Tank Enemy"),
	Agile UMETA(DisplayName = "Agile Enemy"),
	Combat UMETA(DisplayName = "Combat Enemy"),
	Hacker UMETA(DisplayName = "Hacker Enemy")
};

USTRUCT(BlueprintType)
struct BLACKHOLE_API FEnemyStatsData : public FTableRowBase
{
	GENERATED_BODY()

	// === BASIC STATS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats")
	FString EnemyName = "Enemy";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats")
	EEnemyType EnemyType = EEnemyType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	// === MOVEMENT STATS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "50.0"))
	float MovementSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "100.0"))
	float Acceleration = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "100.0"))
	float BrakingDeceleration = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "90.0", ClampMax = "720.0"))
	float RotationRate = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "50.0"))
	float Mass = 100.0f;

	// === COMBAT STATS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Range")
	float AttackRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Range")
	float ChaseRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Range")
	float SightRange = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Range")
	float PreferredCombatDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Range")
	float MinimumEngagementDistance = 100.0f;

	// === DAMAGE STATS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	float BaseDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float AttackSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	float AttackCooldown = 2.0f;

	// === AI BEHAVIOR ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RetreatHealthPercent = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DefensiveHealthPercent = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AggressionLevel = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Timing")
	float ReactionTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Timing")
	float SearchDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Timing")
	float MaxTimeInCombat = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Timing")
	float PatrolWaitTime = 2.0f;

	// === DEFENSIVE ABILITIES ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DodgeChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReactiveDefenseChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Defense")
	float DefensiveCooldown = 2.0f;

	// === SPECIAL ABILITIES ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Special")
	bool bCanDash = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Special", meta = (EditCondition = "bCanDash"))
	float DashCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Special", meta = (EditCondition = "bCanDash"))
	float DashBehindDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Special", meta = (EditCondition = "bCanDash"))
	float DashForce = 3000.0f;

	// === AREA DAMAGE (Tank) ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|AreaDamage")
	bool bHasAreaDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|AreaDamage", meta = (EditCondition = "bHasAreaDamage"))
	float GroundSlamRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|AreaDamage", meta = (EditCondition = "bHasAreaDamage", ClampMin = "1.0", ClampMax = "5.0"))
	float GroundSlamDamageMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|AreaDamage", meta = (EditCondition = "bHasAreaDamage"))
	float GroundSlamKnockbackForce = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|AreaDamage", meta = (EditCondition = "bHasAreaDamage"))
	float GroundSlamCooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|AreaDamage", meta = (EditCondition = "bHasAreaDamage"))
	float GroundSlamStaggerTime = 1.5f;

	// === RANGED ABILITIES (Hacker) ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Ranged")
	bool bHasRangedAttack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Ranged", meta = (EditCondition = "bHasRangedAttack"))
	float RangedAttackRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Ranged", meta = (EditCondition = "bHasRangedAttack"))
	float RangedAttackDamage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Ranged", meta = (EditCondition = "bHasRangedAttack"))
	float RangedAttackCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Ranged", meta = (EditCondition = "bHasRangedAttack"))
	float SafeDistance = 500.0f;

	// === SMASH ABILITY ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Smash")
	bool bHasSmashAbility = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Smash", meta = (EditCondition = "bHasSmashAbility"))
	float SmashDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Smash", meta = (EditCondition = "bHasSmashAbility"))
	float SmashCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Smash", meta = (EditCondition = "bHasSmashAbility"))
	float SmashRange = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Smash", meta = (EditCondition = "bHasSmashAbility"))
	float SmashKnockbackForce = 750.0f;

	// === BLOCK ABILITY ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Block")
	bool bCanBlock = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Block", meta = (EditCondition = "bCanBlock"))
	float BlockDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Block", meta = (EditCondition = "bCanBlock"))
	float BlockCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Block", meta = (EditCondition = "bCanBlock", ClampMin = "0.0", ClampMax = "1.0"))
	float BlockDamageReduction = 0.8f;

	// === DODGE ABILITY ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Dodge")
	bool bCanDodge = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Dodge", meta = (EditCondition = "bCanDodge"))
	float DodgeCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Dodge", meta = (EditCondition = "bCanDodge"))
	float DodgeDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities|Dodge", meta = (EditCondition = "bCanDodge"))
	float DodgeImpulse = 1000.0f;

	// Constructor with default values
	FEnemyStatsData()
	{
		// Default constructor values are set inline above
	}
};

// Manager class to handle data table loading
UCLASS(BlueprintType)
class BLACKHOLE_API UEnemyStatsManager : public UObject
{
	GENERATED_BODY()

public:
	// Static function to get enemy stats from data table
	UFUNCTION(BlueprintCallable, Category = "Enemy Stats", meta = (WorldContext = "WorldContextObject"))
	static FEnemyStatsData GetEnemyStats(UObject* WorldContextObject, UDataTable* EnemyDataTable, FName RowName);

	// Static function to apply stats to an enemy
	UFUNCTION(BlueprintCallable, Category = "Enemy Stats")
	static void ApplyStatsToEnemy(class ABaseEnemy* Enemy, const FEnemyStatsData& Stats);
};