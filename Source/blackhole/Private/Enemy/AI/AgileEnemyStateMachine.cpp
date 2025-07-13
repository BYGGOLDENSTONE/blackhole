#include "Enemy/AI/AgileEnemyStateMachine.h"
#include "Enemy/AI/States/IdleState.h"
#include "Enemy/AI/States/AlertState.h"
#include "Enemy/AI/States/ChaseState.h"
#include "Enemy/AI/States/AgileCombatState.h"
#include "Enemy/AI/States/RetreatState.h"
#include "Enemy/AgileEnemy.h"

void UAgileEnemyStateMachine::BeginPlay()
{
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: BeginPlay started"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: Setting up parameters"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    SetupAgileParameters();
    
    // Delay initialization to ensure BaseEnemy has set the target
    if (GetWorld())
    {
        FTimerHandle InitTimer;
        GetWorld()->GetTimerManager().SetTimer(InitTimer, [this]()
        {
            UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: Delayed Initialize"), 
                GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
                
            // Try to get target from BaseEnemy if we don't have one
            if (!Target && OwnerEnemy)
            {
                if (AActor* EnemyTarget = OwnerEnemy->GetTargetActor())
                {
                    SetTarget(EnemyTarget);
                    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: Got target from BaseEnemy: %s"), 
                        *OwnerEnemy->GetName(), *EnemyTarget->GetName());
                }
            }
            
            Initialize(); // Initialize states after parameters are set
        }, 0.1f, false); // Small delay to ensure proper initialization order
    }
    
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
    URetreatState* RetreatState = NewObject<URetreatState>(this, URetreatState::StaticClass());
    
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: States created, now registering"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    
    // Register states
    RegisterState(EEnemyState::Idle, IdleState);
    RegisterState(EEnemyState::Alert, AlertState);
    RegisterState(EEnemyState::Chase, ChaseState);
    RegisterState(EEnemyState::Combat, CombatState);
    RegisterState(EEnemyState::Retreat, RetreatState);
    
    UE_LOG(LogTemp, Warning, TEXT("%s AgileStateMachine: All states registered"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("NoOwner"));
    
    // Agile enemies don't use defensive state - they dodge instead
}

void UAgileEnemyStateMachine::SetupAgileParameters()
{
    // Agile specific parameters - high mobility, high reaction, medium aggression
    FEnemyAIParameters AgileParams;
    
    // Health thresholds
    AgileParams.RetreatHealthPercent = 0.4f;      // Retreat earlier than tank
    AgileParams.DefensiveHealthPercent = 0.6f;    // More cautious
    
    // Distance thresholds
    AgileParams.AttackRange = 250.0f;              // Slightly longer reach
    AgileParams.ChaseRange = 2200.0f;              // Good chase range
    AgileParams.SightRange = 2500.0f;              
    AgileParams.PreferredCombatDistance = 250.0f;  // Hit and run distance
    
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
    AgileParams.AggressionLevel = 0.7f;            // Medium-high aggression
    
    SetAIParameters(AgileParams);
}