#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/ChaseState.h"
#include "AgileChaseState.generated.h"

UCLASS()
class BLACKHOLE_API UAgileChaseState : public UChaseState
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    
private:
    // Keep distance from player until dash is ready
    void MaintainDistance(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime);
    
    // Preferred distance to maintain
    const float PreferredDistance = 600.0f;
    const float MinDistance = 500.0f;
    const float MaxDistance = 700.0f;
};