#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "StandardEnemy.generated.h"

class USwordAttackComponent;
class UBuilderComponent;
class UBlockComponent;

UCLASS()
class BLACKHOLE_API AStandardEnemy : public ABaseEnemy
{
	GENERATED_BODY()

public:
	AStandardEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void UpdateAIBehavior(float DeltaTime) override;
	
	// Override base enemy capabilities
	virtual bool CanBlock() const override { return true; }
	virtual bool CanDodge() const override { return false; }

	// Combat abilities
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USwordAttackComponent* SwordAttack;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UBlockComponent* BlockAbility;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UBuilderComponent* BuilderComponent;

public:
	// AI Behavior Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (DisplayName = "Chase Range"))
	float ChaseRange = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (DisplayName = "Attack Range"))
	float AttackRange = 180.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (DisplayName = "Has Builder Ability"))
	bool bHasBuilderAbility = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (DisplayName = "Block Chance", ClampMin = "0.0", ClampMax = "1.0"))
	float BlockChance = 0.3f;
	
	// Movement Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (DisplayName = "Movement Speed"))
	float MovementSpeed = 450.0f;
	
	// Get builder component for state machine access
	UFUNCTION(BlueprintPure, Category = "Combat")
	UBuilderComponent* GetBuilderComponent() const { return BuilderComponent; }
	
	// Called when enemy is alerted
	void OnAlerted();

private:
	void MoveTowardsTarget(float DeltaTime);
	void TryAttack();
	void TryBlock();
	float GetDistanceToTarget() const;
	bool IsPlayerAttacking() const;
	
	// Building behavior
	void CheckForBuildOpportunity();
	bool ShouldStartBuilding() const;
	
	// Track player air/wall run time
	float PlayerAirWallRunTime = 0.0f;
	const float AirWallRunBuildThreshold = 3.0f; // Build after 3 seconds of air/wall time
};