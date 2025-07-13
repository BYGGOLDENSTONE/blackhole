#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateBase.h"
#include "ChaseState.generated.h"

UCLASS()
class BLACKHOLE_API UChaseState : public UEnemyStateBase
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    virtual EEnemyState GetStateType() const override { return EEnemyState::Chase; }
    
    virtual void OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage) override;

private:
    float LastPathUpdateTime = 0.0f;
    const float PathUpdateInterval = 0.3f;
    
    void UpdateChaseMovement(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    bool ShouldRetreat(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const;
};