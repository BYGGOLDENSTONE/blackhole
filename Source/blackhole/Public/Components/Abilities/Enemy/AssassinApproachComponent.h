#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/Enemy/EnemyAbilityComponent.h"
#include "AssassinApproachComponent.generated.h"

class UStabAttackComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UAssassinApproachComponent : public UEnemyAbilityComponent
{
	GENERATED_BODY()

public:
	UAssassinApproachComponent();

	// Dash parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assassin Approach", meta = (DisplayName = "Dash Range"))
	float DashRange = 600.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assassin Approach", meta = (DisplayName = "Dash Behind Distance"))
	float DashBehindDistance = 150.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assassin Approach", meta = (DisplayName = "Dash Force", ClampMin = "1000.0", ClampMax = "10000.0"))
	float DashForce = 3000.0f;
	
	// Backstab parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assassin Approach", meta = (DisplayName = "Backstab Damage Multiplier", ClampMin = "1.0", ClampMax = "5.0"))
	float BackstabDamageMultiplier = 2.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assassin Approach", meta = (DisplayName = "Backstab Stagger Duration", ClampMin = "0.0", ClampMax = "3.0"))
	float BackstabStaggerDuration = 1.5f;
	
	// Timing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assassin Approach", meta = (DisplayName = "Attack Delay After Dash", ClampMin = "0.1", ClampMax = "1.0"))
	float AttackDelayAfterDash = 0.3f;

	virtual void Execute() override;
	virtual bool CanExecute() const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UStabAttackComponent* StabAttackComponent;
	
	FTimerHandle DashAttackTimerHandle;
	
	void ExecuteDashBehind();
	void ExecuteBackstab();
	
	// Store target for delayed attack
	TWeakObjectPtr<AActor> DashTarget;
};