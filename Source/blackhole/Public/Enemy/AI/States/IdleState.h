#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateBase.h"
#include "IdleState.generated.h"

UCLASS()
class BLACKHOLE_API UIdleState : public UEnemyStateBase
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    virtual EEnemyState GetStateType() const override { return EEnemyState::Idle; }

private:
    FVector InitialLocation;
    float IdleWanderRadius = 200.0f;
    float NextWanderTime = 0.0f;
};