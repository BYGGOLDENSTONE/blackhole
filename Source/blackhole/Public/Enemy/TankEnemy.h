#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "TankEnemy.generated.h"

class UAreaDamageAbilityComponent;
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
	UAreaDamageAbilityComponent* AreaDamageAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UBlockComponent* BlockAbility;

public:
	// Combat stats - public for data table access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float BlockChance;

protected:
	
	// Shield mesh component - visible when blocking
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* ShieldMesh;

public:
	// The area damage configuration is now handled by AreaDamageAbilityComponent
	// Public methods for state machine access
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetShieldVisible(bool bVisible);
	
	// Get the area damage ability for state machine access
	UFUNCTION(BlueprintPure, Category = "Combat")
	UAreaDamageAbilityComponent* GetAreaDamageAbility() const { return AreaDamageAbility; }

private:
	void MoveTowardsTarget(float DeltaTime);
	void TryAttack();
	void TryBlock();
	float GetDistanceToTarget() const;
	bool IsPlayerAttacking() const;
};