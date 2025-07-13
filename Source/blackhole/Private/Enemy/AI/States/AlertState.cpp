#include "Enemy/AI/States/AlertState.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

void UAlertState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    if (!Enemy || !StateMachine) return;
    
    // Set search center to last known player location
    SearchCenter = StateMachine->GetLastKnownTargetLocation();
    SearchPointsVisited = 0;
    NextSearchPointTime = 0.0f;
    
    // Slow down movement for cautious searching
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = Enemy->GetDefaultWalkSpeed() * 0.6f;
    }
    
    // Play alert animation/sound if available
    Enemy->PlayAlertReaction();
}

void UAlertState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!Enemy || !StateMachine) return;
    
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    
    // Check if we found the player
    if (StateMachine->GetTarget() && HasLineOfSightToPlayer(Enemy))
    {
        float Distance = GetDistanceToPlayer(Enemy);
        
        if (Distance <= Params.ChaseRange)
        {
            // Found player - engage!
            StateMachine->UpdateLastKnownTargetLocation();
            StateMachine->ChangeState(EEnemyState::Chase);
            return;
        }
    }
    
    // Check if search time expired
    if (TimeInState >= Params.SearchDuration)
    {
        // Give up search, return to idle or patrol
        if (Enemy->HasPatrolRoute())
        {
            StateMachine->ChangeState(EEnemyState::Patrol);
        }
        else
        {
            StateMachine->ChangeState(EEnemyState::Idle);
        }
        return;
    }
    
    // Perform search behavior
    SearchBehavior(Enemy, StateMachine);
    
    // Look around while searching (slower rotation)
    if (StateMachine->GetTarget())
    {
        RotateTowardsTarget(Enemy, StateMachine, DeltaTime, 3.0f); // Slower rotation while searching
    }
}

void UAlertState::SearchBehavior(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (TimeInState < NextSearchPointTime) return;
    
    AAIController* AIController = Cast<AAIController>(Enemy->GetController());
    if (!AIController) return;
    
    // Generate search points around last known location
    FNavLocation SearchPoint;
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
    
    if (NavSystem && NavSystem->GetRandomPointInNavigableRadius(SearchCenter, SearchRadius, SearchPoint))
    {
        AIController->MoveToLocation(SearchPoint.Location, 5.0f);
        
        // Debug visualization
        #if WITH_EDITOR
        DrawDebugSphere(Enemy->GetWorld(), SearchPoint.Location, 50.0f, 12, FColor::Yellow, false, 2.0f);
        #endif
        
        SearchPointsVisited++;
        NextSearchPointTime = TimeInState + 2.0f; // Check new point every 2 seconds
        
        // Expand search radius after visiting some points
        if (SearchPointsVisited >= MaxSearchPoints)
        {
            SearchRadius = FMath::Min(SearchRadius * 1.5f, 1000.0f);
            SearchPointsVisited = 0;
        }
    }
}

void UAlertState::OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    // React to sound - update search location
    if (StateMachine && StateMachine->GetTarget())
    {
        SearchCenter = StateMachine->GetTarget()->GetActorLocation();
        SearchRadius = 300.0f; // Reset to smaller radius
        SearchPointsVisited = 0;
    }
}

void UAlertState::OnPlayerUltimateUsed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    // Ultimate abilities make noise - immediately investigate
    if (StateMachine && StateMachine->GetTarget())
    {
        StateMachine->UpdateLastKnownTargetLocation();
        // Stay in alert but move to new location
        SearchCenter = StateMachine->GetLastKnownTargetLocation();
        NextSearchPointTime = TimeInState; // Move immediately
    }
}