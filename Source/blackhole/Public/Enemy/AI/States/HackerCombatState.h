#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/CombatState.h"
#include "HackerCombatState.generated.h"

UCLASS()
class BLACKHOLE_API UHackerCombatState : public UCombatState
{
    GENERATED_BODY()

public:
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;

protected:
    virtual void InitializeCombatActions(ABaseEnemy* Enemy) override;
    virtual void ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName) override;

private:
    void ExecutePulseHack(ABaseEnemy* Enemy);
    void ExecuteMindmeldChannel(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    void ExecuteReposition(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    
    bool ShouldMaintainDistance(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const;
};