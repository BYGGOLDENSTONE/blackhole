#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateBase.h"
#include "AlertState.generated.h"

UCLASS()
class BLACKHOLE_API UAlertState : public UEnemyStateBase
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    virtual EEnemyState GetStateType() const override { return EEnemyState::Alert; }
    
    virtual void OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void OnPlayerUltimateUsed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;

private:
    void SearchBehavior(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    
    FVector SearchCenter;
    float SearchRadius = 500.0f;
    float NextSearchPointTime = 0.0f;
    int32 SearchPointsVisited = 0;
    const int32 MaxSearchPoints = 3;
};