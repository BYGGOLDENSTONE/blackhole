#include "Enemy/AI/TankEnemyStateMachine.h"
#include "Config/GameplayConfig.h"
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
    
    // Initialize immediately - BaseEnemy has already set the target in its BeginPlay
    Initialize();
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
    TankParams.RetreatHealthPercent = 0.0f;      // Never retreat
    TankParams.DefensiveHealthPercent = 0.3f;    // Only defensive when very low
    
    // Distance thresholds
    TankParams.AttackRange = 300.0f;              // Melee range
    TankParams.ChaseRange = GameplayConfig::Enemy::DETECTION_RANGE;  // Full sight range - persistent
    TankParams.SightRange = GameplayConfig::Enemy::DETECTION_RANGE;              
    TankParams.PreferredCombatDistance = 100.0f;  // Get very close
    
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
    TankParams.AggressionLevel = 0.8f;            // High aggression
    
    SetAIParameters(TankParams);
}