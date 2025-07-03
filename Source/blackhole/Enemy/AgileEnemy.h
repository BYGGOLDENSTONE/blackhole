#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
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

	// This enemy can attack and dodge, but CANNOT block
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USmashAbilityComponent* SmashAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UDodgeComponent* DodgeAbility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DodgeChance;

private:
	void MoveTowardsTarget(float DeltaTime);
	void TryAttack();
	void TryDodge();
	float GetDistanceToTarget() const;
};