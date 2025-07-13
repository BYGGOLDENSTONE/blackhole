#include "Enemy/AI/TankEnemyStateMachine.h"
#include "Enemy/AI/States/IdleState.h"
#include "Enemy/AI/States/AlertState.h"
#include "Enemy/AI/States/ChaseState.h"
#include "Enemy/AI/States/TankCombatState.h"
#include "Enemy/AI/States/RetreatState.h"
#include "Enemy/TankEnemy.h"

void UTankEnemyStateMachine::BeginPlay()
{
    Super::BeginPlay();
    SetupTankParameters();
    
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
    } // Initialize states after parameters are set
}

void UTankEnemyStateMachine::InitializeStates()
{
    CreateDefaultStates();
}

void UTankEnemyStateMachine::CreateDefaultStates()
{
    // Create states
    UIdleState* IdleState = NewObject<UIdleState>(this, UIdleState::StaticClass());
    UAlertState* AlertState = NewObject<UAlertState>(this, UAlertState::StaticClass());
    UChaseState* ChaseState = NewObject<UChaseState>(this, UChaseState::StaticClass());
    UTankCombatState* CombatState = NewObject<UTankCombatState>(this, UTankCombatState::StaticClass());
    URetreatState* RetreatState = NewObject<URetreatState>(this, URetreatState::StaticClass());
    
    // Register states
    RegisterState(EEnemyState::Idle, IdleState);
    RegisterState(EEnemyState::Alert, AlertState);
    RegisterState(EEnemyState::Chase, ChaseState);
    RegisterState(EEnemyState::Combat, CombatState);
    RegisterState(EEnemyState::Retreat, RetreatState);
    
    // Tank doesn't have defensive state - uses block during combat instead
}

void UTankEnemyStateMachine::SetupTankParameters()
{
    // Tank specific parameters - high health threshold, low aggression, high defense
    FEnemyAIParameters TankParams;
    
    // Health thresholds
    TankParams.RetreatHealthPercent = 0.2f;      // Only retreat when very low
    TankParams.DefensiveHealthPercent = 0.4f;    // Start being more defensive
    
    // Distance thresholds
    TankParams.AttackRange = 300.0f;              // Melee range
    TankParams.ChaseRange = 2500.0f;              // Full sight range - persistent
    TankParams.SightRange = 2500.0f;              
    TankParams.PreferredCombatDistance = 200.0f;  // Get close for smash
    
    // Timing parameters
    TankParams.ReactionTime = 0.5f;               // Slower reactions - heavy unit
    TankParams.SearchDuration = 4.0f;             // Gives up search quicker
    TankParams.MaxTimeInCombat = 15.0f;           // Can stay in combat longer
    TankParams.PatrolWaitTime = 3.0f;
    
    // Combat parameters
    TankParams.DodgeChance = 0.0f;                // Tanks don't dodge
    TankParams.BlockChance = 0.5f;                // High block chance
    TankParams.ReactiveDefenseChance = 0.6f;      // Good at reactive blocking
    TankParams.AttackCooldown = 2.5f;             // Slower attacks
    TankParams.AbilityCooldown = 4.0f;
    
    // Personality
    TankParams.AggressionLevel = 0.3f;            // Low aggression - defensive
    
    SetAIParameters(TankParams);
}