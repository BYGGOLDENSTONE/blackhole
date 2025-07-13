#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateBase.h"
#include "ChannelingState.generated.h"

UCLASS()
class BLACKHOLE_API UChannelingState : public UEnemyStateBase
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    virtual EEnemyState GetStateType() const override { return EEnemyState::Channeling; }
    
    virtual void OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage) override;
    virtual bool CanTransitionTo(EEnemyState NewState) const override;

private:
    float ChannelDuration = 0.0f;
    float MaxChannelTime = 2.0f; // Reduced from 5.0f to prevent getting stuck
    bool bChannelInterrupted = false;
    
    void StartChanneling(ABaseEnemy* Enemy);
    void StopChanneling(ABaseEnemy* Enemy);
};