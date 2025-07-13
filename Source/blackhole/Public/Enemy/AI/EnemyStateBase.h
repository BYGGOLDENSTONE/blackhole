#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EnemyStates.h"
#include "EnemyStateBase.generated.h"

class ABaseEnemy;
class UEnemyStateMachine;

UCLASS(Abstract, Blueprintable)
class BLACKHOLE_API UEnemyStateBase : public UObject
{
    GENERATED_BODY()

public:
    UEnemyStateBase();

    // State lifecycle
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    virtual void Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine);
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime);

    // State checks
    virtual bool CanTransitionTo(EEnemyState NewState) const { return true; }
    virtual EEnemyState GetStateType() const { return EEnemyState::None; }

    // Combat reactions
    virtual void OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) {}
    virtual void OnPlayerAttacking(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) {}
    virtual void OnPlayerUltimateUsed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) {}
    virtual void OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage) {}

protected:
    // Helper functions
    bool IsPlayerInRange(ABaseEnemy* Enemy, float Range) const;
    bool HasLineOfSightToPlayer(ABaseEnemy* Enemy) const;
    float GetDistanceToPlayer(ABaseEnemy* Enemy) const;
    float GetHealthPercent(ABaseEnemy* Enemy) const;
    bool IsAbilityOnCooldown(ABaseEnemy* Enemy, const FString& AbilityName) const;
    void StartAbilityCooldown(ABaseEnemy* Enemy, const FString& AbilityName, float Duration) const;
    
    // Movement and rotation helpers
    void RotateTowardsTarget(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime, float RotationSpeed = 5.0f) const;

    UPROPERTY()
    float TimeInState = 0.0f;
};