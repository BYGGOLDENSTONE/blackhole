#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "AgileEnemy.generated.h"

class USmashAbilityComponent;
class UDodgeComponent;

UCLASS()
class BLACKHOLE_API AAgileEnemy : public ABaseEnemy
{
	GENERATED_BODY()

public:
	AAgileEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void UpdateAIBehavior(float DeltaTime) override;
	
	// Override base enemy capabilities
	virtual bool CanBlock() const override { return false; }
	virtual bool CanDodge() const override { return true; }

	// This enemy can attack and dodge, but CANNOT block
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USmashAbilityComponent* SmashAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UDodgeComponent* DodgeAbility;

public:
	// Combat stats - public for data table access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DodgeChance;

protected:

public:
	// Attack range - public for state machine access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange;
	// Movement and combat stats - public for state machine access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Movement Speed"))
	float MovementSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Attack Speed Multiplier", ClampMin = "0.1", ClampMax = "3.0"))
	float AttackSpeedMultiplier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Dash Cooldown", ClampMin = "0.5"))
	float DashCooldown;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Dash Behind Distance"))
	float DashBehindDistance;

private:
	void MoveTowardsTarget(float DeltaTime);
	void TryAttack();
	void TryDodge();
	float GetDistanceToTarget() const;
};