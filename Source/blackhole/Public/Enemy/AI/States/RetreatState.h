#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateBase.h"
#include "RetreatState.generated.h"

UCLASS()
class BLACKHOLE_API URetreatState : public UEnemyStateBase
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    virtual EEnemyState GetStateType() const override { return EEnemyState::Retreat; }
    
    virtual void OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage) override;

private:
    FVector RetreatTarget;
    bool bRetreatTargetSet = false;
    float HealStartTime = 0.0f;
    const float HealDelay = 3.0f;
    const float HealRate = 10.0f; // HP per second
    
    void FindRetreatLocation(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    void CallForBackup(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
};