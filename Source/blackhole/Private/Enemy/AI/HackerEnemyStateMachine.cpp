#include "Enemy/AI/HackerEnemyStateMachine.h"
#include "Config/GameplayConfig.h"
#include "Enemy/AI/States/IdleState.h"
#include "Enemy/AI/States/AlertState.h"
#include "Enemy/AI/States/ChaseState.h"
#include "Enemy/AI/States/HackerCombatState.h"
#include "Enemy/AI/States/ChannelingState.h"
#include "Enemy/AI/States/RetreatState.h"
#include "Enemy/HackerEnemy.h"

void UHackerEnemyStateMachine::BeginPlay()
{
    Super::BeginPlay();
    SetupHackerParameters();
    
    // Initialize immediately - BaseEnemy has already set the target in its BeginPlay
    Initialize(); // Initialize states after parameters are set
}

void UHackerEnemyStateMachine::InitializeStates()
{
    CreateDefaultStates();
}

void UHackerEnemyStateMachine::CreateDefaultStates()
{
    // Create states
    UIdleState* IdleState = NewObject<UIdleState>(this, UIdleState::StaticClass());
    UAlertState* AlertState = NewObject<UAlertState>(this, UAlertState::StaticClass());
    UChaseState* ChaseState = NewObject<UChaseState>(this, UChaseState::StaticClass());
    UHackerCombatState* CombatState = NewObject<UHackerCombatState>(this, UHackerCombatState::StaticClass());
    UChannelingState* ChannelingState = NewObject<UChannelingState>(this, UChannelingState::StaticClass());
    URetreatState* RetreatState = NewObject<URetreatState>(this, URetreatState::StaticClass());
    
    // Register states
    RegisterState(EEnemyState::Idle, IdleState);
    RegisterState(EEnemyState::Alert, AlertState);
    RegisterState(EEnemyState::Chase, ChaseState);
    RegisterState(EEnemyState::Combat, CombatState);
    RegisterState(EEnemyState::Channeling, ChannelingState);
    RegisterState(EEnemyState::Retreat, RetreatState);
    
    // Hacker has unique channeling state for mindmeld
}

void UHackerEnemyStateMachine::SetupHackerParameters()
{
    // Hacker specific parameters - ranged, low health, high caution
    FEnemyAIParameters HackerParams;
    
    // Health thresholds
    HackerParams.RetreatHealthPercent = 0.4f;      // Retreat at 40% health
    HackerParams.DefensiveHealthPercent = 0.7f;    // Very cautious
    
    // Distance thresholds
    HackerParams.AttackRange = 1200.0f;            // Long range for mindmeld
    HackerParams.ChaseRange = 1800.0f;             // Limited chase - prefers range
    HackerParams.SightRange = GameplayConfig::Enemy::DETECTION_RANGE;              
    HackerParams.PreferredCombatDistance = 800.0f; // Keep distance
    
    // Timing parameters
    HackerParams.ReactionTime = 0.4f;              // Moderate reactions
    HackerParams.SearchDuration = 5.0f;            // Gives up quicker
    HackerParams.MaxTimeInCombat = 6.0f;           // Doesn't like prolonged combat
    HackerParams.PatrolWaitTime = 2.5f;
    
    // Combat parameters
    HackerParams.DodgeChance = 0.2f;               // Some evasion
    HackerParams.BlockChance = 0.0f;               // Can't block
    HackerParams.ReactiveDefenseChance = 0.3f;     // Less reactive - focused on abilities
    HackerParams.AttackCooldown = 3.0f;            // Slower attacks
    HackerParams.AbilityCooldown = 5.0f;           // Mindmeld cooldown
    
    // Personality
    HackerParams.AggressionLevel = 0.2f;           // Low aggression - support role
    
    SetAIParameters(HackerParams);
}