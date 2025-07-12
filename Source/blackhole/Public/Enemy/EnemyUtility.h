#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyUtility.generated.h"

class ABaseEnemy;
class ABlackholePlayerCharacter;

/**
 * Utility functions for enemy behavior to reduce code duplication
 */
UCLASS()
class BLACKHOLE_API UEnemyUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Movement functions
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static void MoveTowardsTarget(ABaseEnemy* Enemy, AActor* Target, float DeltaTime, float MoveSpeed = 300.0f);
	
	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	static float GetDistanceToTarget(const ABaseEnemy* Enemy, const AActor* Target);
	
	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	static FVector GetDirectionToTarget(const ABaseEnemy* Enemy, const AActor* Target);
	
	// Combat functions
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	static bool TryBasicAttack(ABaseEnemy* Enemy, float AttackRange = 200.0f, float Damage = 10.0f);
	
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	static bool IsInAttackRange(const ABaseEnemy* Enemy, float AttackRange = 200.0f);
	
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	static bool HasLineOfSightToPlayer(const ABaseEnemy* Enemy);
	
	// Player detection
	UFUNCTION(BlueprintCallable, Category = "Enemy|Detection")
	static ABlackholePlayerCharacter* FindPlayerInWorld(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "Enemy|Detection")
	static bool IsPlayerInDetectionRange(const ABaseEnemy* Enemy, float DetectionRange = 2500.0f);
	
	// State helpers
	UFUNCTION(BlueprintCallable, Category = "Enemy|State")
	static void StartCombatState(ABaseEnemy* Enemy);
	
	UFUNCTION(BlueprintCallable, Category = "Enemy|State")
	static void EndCombatState(ABaseEnemy* Enemy);
};