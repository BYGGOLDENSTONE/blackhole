#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/CombatState.h"
#include "AgileCombatState.generated.h"

UCLASS()
class BLACKHOLE_API UAgileCombatState : public UCombatState
{
    GENERATED_BODY()

public:
    virtual void Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;

protected:
    virtual void InitializeCombatActions(ABaseEnemy* Enemy) override;
    virtual void ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName) override;
    
    virtual void OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;

private:
    void ExecuteQuickStrike(ABaseEnemy* Enemy);
    void ExecuteDodgeManeuver(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    void ExecuteDashAttack(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    
    bool bLastActionWasDodge = false;
    
    // Timer handle for dash attack
    FTimerHandle DashAttackTimerHandle;
    
    // Circle strafe behavior
    bool bIsCircleStrafing = false;
    float CircleStrafeDirection = 1.0f; // 1 or -1 for left/right
    float TimeSinceDirectionChange = 0.0f;
    void UpdateCircleStrafe(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime);
};