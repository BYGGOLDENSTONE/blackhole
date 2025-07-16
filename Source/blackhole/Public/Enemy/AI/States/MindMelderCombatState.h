#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/CombatState.h"
#include "MindMelderCombatState.generated.h"

UCLASS()
class BLACKHOLE_API UMindMelderCombatState : public UCombatState
{
    GENERATED_BODY()

protected:
    virtual void InitializeCombatActions(ABaseEnemy* Enemy) override;
    virtual void ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName) override;
    
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;

private:
    void ExecutePowerfulMindmeld(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    void ExecuteDodge(ABaseEnemy* Enemy);
    void MaintainSafeDistance(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime);
    
    bool bIsChanneling;
    float TimeAtSafeDistance;
};