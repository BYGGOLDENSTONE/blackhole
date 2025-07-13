#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateBase.h"
#include "CombatState.generated.h"

USTRUCT()
struct FCombatAction
{
    GENERATED_BODY()

    FString ActionName;
    float Weight = 1.0f;
    float LastUsedTime = -999.0f;
    float Cooldown = 2.0f;
    float MinRange = 0.0f;
    float MaxRange = 300.0f;
};

UCLASS()
class BLACKHOLE_API UCombatState : public UEnemyStateBase
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    virtual EEnemyState GetStateType() const override { return EEnemyState::Combat; }
    
    virtual void OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void OnPlayerAttacking(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage) override;

protected:
    // Combat action management
    virtual void InitializeCombatActions(ABaseEnemy* Enemy) {}
    virtual void ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName) {}
    
    FString SelectCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    void AddCombatAction(const FString& Name, float Weight, float Cooldown, float MinRange, float MaxRange);
    
    TMap<FString, FCombatAction> CombatActions;
    
private:
    float NextActionTime = 0.0f;
    float LastAttackTime = 0.0f;
    bool bIsExecutingAction = false;
    
    void UpdateCombatPosition(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    bool ShouldDefend(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const;
};