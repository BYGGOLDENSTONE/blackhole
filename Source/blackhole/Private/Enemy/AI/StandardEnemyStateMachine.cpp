#include "Enemy/AI/StandardEnemyStateMachine.h"
#include "Config/GameplayConfig.h"
#include "Enemy/AI/States/IdleState.h"
#include "Enemy/AI/States/AlertState.h"
#include "Enemy/AI/States/ChaseState.h"
#include "Enemy/AI/States/StandardCombatState.h"
#include "Enemy/AI/States/RetreatState.h"
#include "Enemy/StandardEnemy.h"

void UStandardEnemyStateMachine::BeginPlay()
{
    Super::BeginPlay();
    SetupStandardParameters();
    
    // Initialize immediately - BaseEnemy has already set the target in its BeginPlay
    Initialize();
}

void UStandardEnemyStateMachine::InitializeStates()
{
    CreateDefaultStates();
}

void UStandardEnemyStateMachine::CreateDefaultStates()
{
    // Create states
    UIdleState* IdleState = NewObject<UIdleState>(this, UIdleState::StaticClass());
    UAlertState* AlertState = NewObject<UAlertState>(this, UAlertState::StaticClass());
    UChaseState* ChaseState = NewObject<UChaseState>(this, UChaseState::StaticClass());
    UStandardCombatState* CombatState = NewObject<UStandardCombatState>(this, UStandardCombatState::StaticClass());
    URetreatState* RetreatState = NewObject<URetreatState>(this, URetreatState::StaticClass());
    
    // Register states
    RegisterState(EEnemyState::Idle, IdleState);
    RegisterState(EEnemyState::Alert, AlertState);
    RegisterState(EEnemyState::Chase, ChaseState);
    RegisterState(EEnemyState::Combat, CombatState);
    RegisterState(EEnemyState::Retreat, RetreatState);
}

void UStandardEnemyStateMachine::SetupStandardParameters()
{
    // Standard enemy parameters - balanced unit
    FEnemyAIParameters StandardParams;
    
    // Health thresholds
    StandardParams.RetreatHealthPercent = 0.2f;      // Retreat at low health
    StandardParams.DefensiveHealthPercent = 0.4f;    // Defensive when hurt
    
    // Distance thresholds
    StandardParams.AttackRange = 180.0f;              // Sword range
    StandardParams.ChaseRange = 1000.0f;              // Standard chase range
    StandardParams.SightRange = GameplayConfig::Enemy::DETECTION_RANGE;              
    StandardParams.PreferredCombatDistance = 150.0f;  // Sword fighting distance
    
    // Timing parameters
    StandardParams.ReactionTime = 0.3f;               // Average reactions
    StandardParams.SearchDuration = 5.0f;             // Standard search time
    StandardParams.MaxTimeInCombat = 10.0f;           // Normal combat endurance
    StandardParams.PatrolWaitTime = 2.0f;
    
    // Combat parameters
    StandardParams.DodgeChance = 0.0f;                // Standard enemies don't dodge
    StandardParams.BlockChance = 0.3f;                // Moderate block chance
    StandardParams.ReactiveDefenseChance = 0.4f;      // Average reactive defense
    StandardParams.AttackCooldown = 1.5f;             // Standard attack speed
    StandardParams.AbilityCooldown = 3.0f;
    
    // Personality
    StandardParams.AggressionLevel = 0.5f;            // Balanced aggression
    
    SetAIParameters(StandardParams);
}