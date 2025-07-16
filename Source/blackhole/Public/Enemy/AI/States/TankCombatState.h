#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/CombatState.h"
#include "TankCombatState.generated.h"

UCLASS()
class BLACKHOLE_API UTankCombatState : public UCombatState
{
    GENERATED_BODY()

protected:
    virtual void InitializeCombatActions(ABaseEnemy* Enemy) override;
    virtual void ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName) override;

private:
    void ExecuteSmashAttack(ABaseEnemy* Enemy);
    void ExecuteBlockStance(ABaseEnemy* Enemy);
    void ExecuteGroundSlam(ABaseEnemy* Enemy);
    void ExecuteCharge(ABaseEnemy* Enemy);
};