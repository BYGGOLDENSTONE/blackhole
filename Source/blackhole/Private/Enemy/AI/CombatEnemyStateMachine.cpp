#include "Enemy/AI/CombatEnemyStateMachine.h"
#include "Enemy/AI/States/IdleState.h"
#include "Enemy/AI/States/AlertState.h"
#include "Enemy/AI/States/ChaseState.h"
#include "Enemy/AI/States/VersatileCombatState.h"
#include "Enemy/AI/States/RetreatState.h"
#include "Enemy/CombatEnemy.h"

void UCombatEnemyStateMachine::BeginPlay()
{
    Super::BeginPlay();
    SetupCombatParameters();
    
    // Delay initialization to ensure BaseEnemy has set the target
    if (GetWorld())
    {
        FTimerHandle InitTimer;
        GetWorld()->GetTimerManager().SetTimer(InitTimer, [this]()
        {
            // Try to get target from BaseEnemy if we don't have one
            if (!Target && OwnerEnemy)
            {
                if (AActor* EnemyTarget = OwnerEnemy->GetTargetActor())
                {
                    SetTarget(EnemyTarget);
                }
            }
            Initialize();
        }, 0.1f, false);
    }
}

void UCombatEnemyStateMachine::InitializeStates()
{
    CreateDefaultStates();
}

void UCombatEnemyStateMachine::CreateDefaultStates()
{
    // Create states
    UIdleState* IdleState = NewObject<UIdleState>(this, UIdleState::StaticClass());
    UAlertState* AlertState = NewObject<UAlertState>(this, UAlertState::StaticClass());
    UChaseState* ChaseState = NewObject<UChaseState>(this, UChaseState::StaticClass());
    UVersatileCombatState* CombatState = NewObject<UVersatileCombatState>(this, UVersatileCombatState::StaticClass());
    URetreatState* RetreatState = NewObject<URetreatState>(this, URetreatState::StaticClass());
    
    // Register states
    RegisterState(EEnemyState::Idle, IdleState);
    RegisterState(EEnemyState::Alert, AlertState);
    RegisterState(EEnemyState::Chase, ChaseState);
    RegisterState(EEnemyState::Combat, CombatState);
    RegisterState(EEnemyState::Retreat, RetreatState);
    
    // Combat enemy is versatile - uses all states effectively
}

void UCombatEnemyStateMachine::SetupCombatParameters()
{
    // Combat enemy - balanced all-rounder
    FEnemyAIParameters CombatParams;
    
    // Health thresholds
    CombatParams.RetreatHealthPercent = 0.3f;      // Balanced retreat threshold
    CombatParams.DefensiveHealthPercent = 0.5f;    // Standard defensive threshold
    
    // Distance thresholds
    CombatParams.AttackRange = 300.0f;             // Standard melee range
    CombatParams.ChaseRange = 2000.0f;             // Good chase capability
    CombatParams.SightRange = 2500.0f;              
    CombatParams.PreferredCombatDistance = 250.0f; // Flexible positioning
    
    // Timing parameters
    CombatParams.ReactionTime = 0.3f;              // Balanced reactions
    CombatParams.SearchDuration = 6.0f;            // Standard search time
    CombatParams.MaxTimeInCombat = 10.0f;          // Can sustain combat
    CombatParams.PatrolWaitTime = 2.0f;
    
    // Combat parameters
    CombatParams.DodgeChance = 0.25f;              // Can dodge
    CombatParams.BlockChance = 0.25f;              // Can block
    CombatParams.ReactiveDefenseChance = 0.5f;     // Balanced defense
    CombatParams.AttackCooldown = 2.0f;            // Standard attack rate
    CombatParams.AbilityCooldown = 3.5f;           // Moderate ability use
    
    // Personality
    CombatParams.AggressionLevel = 0.5f;           // Perfectly balanced
    
    SetAIParameters(CombatParams);
}