#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/CombatState.h"
#include "StandardCombatState.generated.h"

UCLASS()
class BLACKHOLE_API UStandardCombatState : public UCombatState
{
    GENERATED_BODY()

protected:
    virtual void InitializeCombatActions(ABaseEnemy* Enemy) override;
    virtual void ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName) override;
    
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;

private:
    void ExecuteSwordAttack(ABaseEnemy* Enemy);
    void ExecuteBlock(ABaseEnemy* Enemy);
    void CheckBuildingOpportunity(ABaseEnemy* Enemy);
    
    bool bHasCheckedBuilding;
    float TimeSinceLastBuildCheck;
};