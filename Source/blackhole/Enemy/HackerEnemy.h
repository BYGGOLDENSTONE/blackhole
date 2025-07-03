#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
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
	virtual void OnDeath() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UMindmeldComponent* MindmeldAbility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MindmeldRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SafeDistance;

private:
	void MaintainLineOfSight(float DeltaTime);
	bool HasLineOfSightToTarget() const;
	float GetDistanceToTarget() const;
};