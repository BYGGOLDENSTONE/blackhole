#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/CombatState.h"
#include "VersatileCombatState.generated.h"

UCLASS()
class BLACKHOLE_API UVersatileCombatState : public UCombatState
{
    GENERATED_BODY()

protected:
    virtual void InitializeCombatActions(ABaseEnemy* Enemy) override;
    virtual void ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName) override;
    
    virtual void OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void OnPlayerAttacking(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;

public:
    virtual void Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;

private:
    void ExecuteSmashAttack(ABaseEnemy* Enemy);
    void ExecuteBlockDefense(ABaseEnemy* Enemy);
    void ExecuteDodgeRoll(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    void ExecuteComboStrike(ABaseEnemy* Enemy);
    
    // Track combo state
    bool bIsInCombo = false;
    int32 ComboHitCount = 0;
    
    // Timer handles for combo
    TArray<FTimerHandle> ComboTimerHandles;
};