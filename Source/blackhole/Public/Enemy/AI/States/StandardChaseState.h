#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/ChaseState.h"
#include "StandardChaseState.generated.h"

UCLASS()
class BLACKHOLE_API UStandardChaseState : public UChaseState
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    
protected:
    // Time tracking for building opportunity
    float TimeSinceChaseStart = 0.0f;
    const float BuildAfterChaseTime = 4.0f; // Build after 4 seconds of chasing
    bool bHasTriggeredBuilding = false;
    
    void CheckBuildingOpportunity(ABaseEnemy* Enemy);
};