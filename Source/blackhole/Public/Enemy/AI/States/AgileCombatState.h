#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/States/CombatState.h"
#include "AgileCombatState.generated.h"

// Assassin combat phases
UENUM()
enum class EAgileCombatPhase : uint8
{
    Approaching,    // Moving to 500 range
    DashAttack,     // Executing dash behind + backstab
    Retreating,     // Moving to 700 range  
    Maintaining     // Keeping distance until dash cooldown
};

UCLASS()
class BLACKHOLE_API UAgileCombatState : public UCombatState
{
    GENERATED_BODY()

public:
    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
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
    
    // Assassin behavior
    void UpdateAssassinBehavior(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime);
    bool ShouldRetreat(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const;
    FVector GetRetreatPosition(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const;
    
    bool bLastActionWasDodge = false;
    
    // Timer handle for dash attack
    FTimerHandle DashAttackTimerHandle;
    
    // Assassin state tracking
    EAgileCombatPhase CurrentPhase = EAgileCombatPhase::Approaching;
    bool bHasExecutedBackstab = false;
    float TimeInCurrentPhase = 0.0f;
    
    // Combat distances
    const float DashEngageRange = 500.0f;
    const float RetreatDistance = 700.0f;
    const float MaintainDistanceMin = 650.0f;
    const float MaintainDistanceMax = 750.0f;
    
    // Circle strafe behavior (kept for dodging)
    bool bIsCircleStrafing = false;
    float CircleStrafeDirection = 1.0f; // 1 or -1 for left/right
    float TimeSinceDirectionChange = 0.0f;
    void UpdateCircleStrafe(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime);
    
    // Force attack timer
    float TimeInMaintainPhase = 0.0f;
    const float MaxMaintainTime = 5.0f; // Force attack after 5 seconds
};