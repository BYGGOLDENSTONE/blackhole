#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
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

private:
	void MoveTowardsTarget(float DeltaTime);
	void TryAttack();
	void TryBlock();
	float GetDistanceToTarget() const;
	bool IsPlayerAttacking() const;
};