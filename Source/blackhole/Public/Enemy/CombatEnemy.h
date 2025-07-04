#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "CombatEnemy.generated.h"

class USmashAbilityComponent;
class UBlockComponent;
class UDodgeComponent;
class UStaticMeshComponent;

UCLASS()
class BLACKHOLE_API ACombatEnemy : public ABaseEnemy
{
	GENERATED_BODY()

public:
	ACombatEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void UpdateAIBehavior(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USmashAbilityComponent* SmashAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UBlockComponent* BlockAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UDodgeComponent* DodgeAbility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseRange;
	
	// Shield mesh component - visible when blocking
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* ShieldMesh;

private:
	void MoveTowardsTarget(float DeltaTime);
	void TryAttack();
	float GetDistanceToTarget() const;
};