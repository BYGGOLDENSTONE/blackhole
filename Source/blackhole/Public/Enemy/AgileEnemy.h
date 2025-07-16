#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "AgileEnemy.generated.h"

class UStabAttackComponent;
class UAssassinApproachComponent;
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

	// Combat abilities
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UStabAttackComponent* StabAttack;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAssassinApproachComponent* AssassinApproach;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UDodgeComponent* DodgeAbility;

public:
	// AI Behavior Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (DisplayName = "Chase Range"))
	float ChaseRange = 1200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (DisplayName = "Attack Range"))
	float AttackRange = 150.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (DisplayName = "Maintain Distance Min", ClampMin = "300.0", ClampMax = "1000.0"))
	float MaintainDistanceMin = 450.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (DisplayName = "Maintain Distance Max", ClampMin = "400.0", ClampMax = "1200.0"))
	float MaintainDistanceMax = 550.0f;

	// Movement Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (DisplayName = "Movement Speed"))
	float MovementSpeed = 600.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (DisplayName = "Retreat Duration", ClampMin = "1.0", ClampMax = "10.0"))
	float RetreatDuration = 6.0f;
	
	// Combat Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Dodge Chance", ClampMin = "0.0", ClampMax = "1.0"))
	float DodgeChance = 0.4f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Attack Speed Multiplier", ClampMin = "0.1", ClampMax = "3.0"))
	float AttackSpeedMultiplier = 1.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Dash Cooldown", ClampMin = "0.5"))
	float DashCooldown = 2.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Dash Behind Distance"))
	float DashBehindDistance = 250.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Backstab Damage Multiplier", ClampMin = "1.0", ClampMax = "5.0"))
	float BackstabDamageMultiplier = 2.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Backstab Stagger Duration", ClampMin = "0.1", ClampMax = "3.0"))
	float BackstabStaggerDuration = 1.5f;

private:
	void MoveTowardsTarget(float DeltaTime);
	void TryAttack();
	void TryDodge();
	float GetDistanceToTarget() const;
};