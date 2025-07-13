#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "HackerEnemy.generated.h"

class UMindmeldComponent;

UCLASS()
class BLACKHOLE_API AHackerEnemy : public ABaseEnemy
{
	GENERATED_BODY()

public:
	AHackerEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void UpdateAIBehavior(float DeltaTime) override;
	
	// Override base enemy capabilities - can only dodge
	virtual bool CanBlock() const override { return false; }
	virtual bool CanDodge() const override { return true; }
	virtual void OnDeath() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UMindmeldComponent* MindmeldAbility;

public:
	// Combat stats - public for data table access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MindmeldRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SafeDistance;

protected:

private:
	void MaintainLineOfSight(float DeltaTime);
	bool HasLineOfSightToTarget() const;
	float GetDistanceToTarget() const;
};