#include "Enemy/AI/MindMelderStateMachine.h"
#include "Config/GameplayConfig.h"
#include "Enemy/AI/States/IdleState.h"
#include "Enemy/AI/States/AlertState.h"
#include "Enemy/AI/States/ChaseState.h"
#include "Enemy/AI/States/MindMelderCombatState.h"
#include "Enemy/AI/States/RetreatState.h"
#include "Enemy/AI/States/ChannelingState.h"
#include "Enemy/MindMelderEnemy.h"

void UMindMelderStateMachine::BeginPlay()
{
    Super::BeginPlay();
    SetupMindMelderParameters();
    
    // Initialize immediately - BaseEnemy has already set the target in its BeginPlay
    Initialize();
}

void UMindMelderStateMachine::InitializeStates()
{
    CreateDefaultStates();
}

void UMindMelderStateMachine::CreateDefaultStates()
{
    // Create states
    UIdleState* IdleState = NewObject<UIdleState>(this, UIdleState::StaticClass());
    UAlertState* AlertState = NewObject<UAlertState>(this, UAlertState::StaticClass());
    UChaseState* ChaseState = NewObject<UChaseState>(this, UChaseState::StaticClass());
    UMindMelderCombatState* CombatState = NewObject<UMindMelderCombatState>(this, UMindMelderCombatState::StaticClass());
    URetreatState* RetreatState = NewObject<URetreatState>(this, URetreatState::StaticClass());
    UChannelingState* ChannelingState = NewObject<UChannelingState>(this, UChannelingState::StaticClass());
    
    // Register states
    RegisterState(EEnemyState::Idle, IdleState);
    RegisterState(EEnemyState::Alert, AlertState);
    RegisterState(EEnemyState::Chase, ChaseState);
    RegisterState(EEnemyState::Combat, CombatState);
    RegisterState(EEnemyState::Retreat, RetreatState);
    RegisterState(EEnemyState::Channeling, ChannelingState);
}

void UMindMelderStateMachine::SetupMindMelderParameters()
{
    // Mind Melder parameters - long range caster, very fragile
    FEnemyAIParameters MindMelderParams;
    
    // Health thresholds
    MindMelderParams.RetreatHealthPercent = 0.5f;      // Retreat at half health
    MindMelderParams.DefensiveHealthPercent = 0.7f;    // Defensive when slightly hurt
    
    // Distance thresholds
    MindMelderParams.AttackRange = 3000.0f;             // Very long range
    MindMelderParams.ChaseRange = 2500.0f;              // Maintain distance
    MindMelderParams.SightRange = GameplayConfig::Enemy::DETECTION_RANGE;              
    MindMelderParams.PreferredCombatDistance = 2000.0f; // Stay far away
    
    // Timing parameters
    MindMelderParams.ReactionTime = 0.2f;               // Quick to react (to run away)
    MindMelderParams.SearchDuration = 8.0f;             // Searches longer
    MindMelderParams.MaxTimeInCombat = 40.0f;           // Stays in combat for channeling
    MindMelderParams.PatrolWaitTime = 3.0f;
    
    // Combat parameters
    MindMelderParams.DodgeChance = 0.6f;                // High dodge chance
    MindMelderParams.BlockChance = 0.0f;                // Can't block
    MindMelderParams.ReactiveDefenseChance = 0.8f;      // Very defensive
    MindMelderParams.AttackCooldown = 60.0f;            // Long cooldown (mindmeld)
    MindMelderParams.AbilityCooldown = 60.0f;
    
    // Personality
    MindMelderParams.AggressionLevel = 0.2f;            // Very low aggression
    
    SetAIParameters(MindMelderParams);
}