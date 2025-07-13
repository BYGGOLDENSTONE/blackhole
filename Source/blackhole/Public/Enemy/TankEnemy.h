#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "TankEnemy.generated.h"

class USmashAbilityComponent;
class UBlockComponent;
class UStaticMeshComponent;

UCLASS()
class BLACKHOLE_API ATankEnemy : public ABaseEnemy
{
	GENERATED_BODY()

public:
	ATankEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void UpdateAIBehavior(float DeltaTime) override;
	
	// Override base enemy capabilities
	virtual bool CanBlock() const override { return true; }
	virtual bool CanDodge() const override { return false; }

	// This enemy can attack and block, but CANNOT dodge
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USmashAbilityComponent* SmashAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UBlockComponent* BlockAbility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float BlockChance;
	
	// Shield mesh component - visible when blocking
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* ShieldMesh;
	
	// Area damage configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Ground Slam Radius"))
	float GroundSlamRadius;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Ground Slam Damage Multiplier", ClampMin = "1.0", ClampMax = "5.0"))
	float GroundSlamDamageMultiplier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Ground Slam Knockback Force"))
	float GroundSlamKnockbackForce;

public:
	// Public methods for state machine access
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetShieldVisible(bool bVisible);
	
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetDamage() const;
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetDamage(float NewDamage);

private:
	void MoveTowardsTarget(float DeltaTime);
	void TryAttack();
	void TryBlock();
	float GetDistanceToTarget() const;
	bool IsPlayerAttacking() const;
};