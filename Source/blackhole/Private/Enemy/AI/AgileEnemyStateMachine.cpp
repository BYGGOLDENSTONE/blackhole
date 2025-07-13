#include "Enemy/AI/AgileEnemyStateMachine.h"
#include "Enemy/AI/States/IdleState.h"
#include "Enemy/AI/States/AlertState.h"
#include "Enemy/AI/States/ChaseState.h"
#include "Enemy/AI/States/AgileCombatState.h"
#include "Enemy/AgileEnemy.h"
#include "Config/GameplayConfig.h"

void UAgileEnemyStateMachine::BeginPlay()
{
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: BeginPlay started"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: Setting up parameters"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    SetupAgileParameters();
    
    // Initialize immediately - BaseEnemy has already set the target in its BeginPlay
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: Calling Initialize immediately"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    Initialize();
    
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: BeginPlay complete"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
}

void UAgileEnemyStateMachine::InitializeStates()
{
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: InitializeStates called"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    CreateDefaultStates();
}

void UAgileEnemyStateMachine::CreateDefaultStates()
{
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: CreateDefaultStates started"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    
    // Create states
    UIdleState* IdleState = NewObject<UIdleState>(this, UIdleState::StaticClass());
    UAlertState* AlertState = NewObject<UAlertState>(this, UAlertState::StaticClass());
    UChaseState* ChaseState = NewObject<UChaseState>(this, UChaseState::StaticClass());
    UAgileCombatState* CombatState = NewObject<UAgileCombatState>(this, UAgileCombatState::StaticClass());
    // No retreat state for agile enemies - they never retreat
    
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: States created, now registering"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    
    // Register states
    RegisterState(EEnemyState::Idle, IdleState);
    RegisterState(EEnemyState::Alert, AlertState);
    RegisterState(EEnemyState::Chase, ChaseState);
    RegisterState(EEnemyState::Combat, CombatState);
    // No retreat state registered - agile enemies fight to the death
    
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: All states registered"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    
    // Agile enemies don't use defensive state - they dodge instead
}

void UAgileEnemyStateMachine::SetupAgileParameters()
{
    // Agile specific parameters - high mobility, high reaction, medium aggression
    FEnemyAIParameters AgileParams;
    
    // Health thresholds
    AgileParams.RetreatHealthPercent = 0.0f;      // Never retreat
    AgileParams.DefensiveHealthPercent = 0.0f;    // Always aggressive
    
    // Distance thresholds  
    AAgileEnemy* AgileOwner = Cast<AAgileEnemy>(GetOwner());
    float AttackRange = AgileOwner ? AgileOwner->AttackRange : 200.0f;
    
    AgileParams.AttackRange = AttackRange;         // Use configurable attack range
    AgileParams.ChaseRange = 3000.0f;              // Very persistent chaser
    AgileParams.SightRange = GameplayConfig::Enemy::DETECTION_RANGE;              
    AgileParams.PreferredCombatDistance = 100.0f;  // Get very close
    
    // Timing parameters
    AgileParams.ReactionTime = 0.2f;               // Very fast reactions
    AgileParams.SearchDuration = 8.0f;             // Persistent searcher
    AgileParams.MaxTimeInCombat = 8.0f;            // Hit and run style
    AgileParams.PatrolWaitTime = 1.5f;             // Restless
    
    // Combat parameters
    AgileParams.DodgeChance = 0.3f;                // Base 30% dodge
    AgileParams.BlockChance = 0.0f;                // Can't block
    AgileParams.ReactiveDefenseChance = 0.7f;      // Very reactive
    AgileParams.AttackCooldown = 1.5f;             // Fast attacks
    AgileParams.AbilityCooldown = 3.0f;            // Quick ability use
    
    // Personality
    AgileParams.AggressionLevel = 1.0f;            // Maximum aggression
    
    SetAIParameters(AgileParams);
}