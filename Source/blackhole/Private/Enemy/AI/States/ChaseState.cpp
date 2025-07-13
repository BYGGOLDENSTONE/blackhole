#include "Enemy/AI/States/ChaseState.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

void UChaseState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    if (!Enemy) return;
    
    // Set chase speed based on aggression
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    float SpeedMultiplier = 1.0f + (Params.AggressionLevel * 0.3f); // Up to 30% faster for aggressive enemies
    
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = Enemy->GetDefaultWalkSpeed() * SpeedMultiplier;
    }
    
    LastPathUpdateTime = 0.0f;
}

void UChaseState::Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Exit(Enemy, StateMachine);
    
    // Reset to normal speed
    if (Enemy && Enemy->GetCharacterMovement())
    {
        Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->GetDefaultWalkSpeed();
    }
}

void UChaseState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!Enemy || !StateMachine || !StateMachine->GetTarget()) return;
    
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    float Distance = GetDistanceToPlayer(Enemy);
    
    // Check if we should retreat
    if (ShouldRetreat(Enemy, StateMachine))
    {
        StateMachine->ChangeState(EEnemyState::Retreat);
        return;
    }
    
    // Check if in combat range
    if (Distance <= Params.AttackRange)
    {
        StateMachine->ChangeState(EEnemyState::Combat);
        return;
    }
    
    // Check if lost sight and out of chase range
    if (!HasLineOfSightToPlayer(Enemy) || Distance > Params.ChaseRange * 1.5f)
    {
        // Lost target - go to alert state
        StateMachine->ChangeState(EEnemyState::Alert);
        return;
    }
    
    // Update chase movement
    UpdateChaseMovement(Enemy, StateMachine);
    
    // Rotate towards target while chasing
    RotateTowardsTarget(Enemy, StateMachine, DeltaTime, 5.0f);
    
    // Debug: Direct movement test
    if (Enemy->GetCharacterMovement())
    {
        FVector CurrentVelocity = Enemy->GetCharacterMovement()->Velocity;
        if (CurrentVelocity.IsNearlyZero())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: WARNING - Velocity is zero in chase state!"), *Enemy->GetName());
        }
    }
}

void UChaseState::UpdateChaseMovement(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (TimeInState - LastPathUpdateTime < PathUpdateInterval) return;
    
    AAIController* AIController = Cast<AAIController>(Enemy->GetController());
    AActor* Target = StateMachine->GetTarget();
    
    if (!AIController)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: No AIController in UpdateChaseMovement!"), *Enemy->GetName());
        return;
    }
    
    if (!Target)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: No Target in UpdateChaseMovement!"), *Enemy->GetName());
        return;
    }
    
    // Different chase behaviors based on enemy type
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    
    // Predict player movement for smarter chasing
    FVector TargetLocation = Target->GetActorLocation();
    if (ACharacter* PlayerChar = Cast<ACharacter>(Target))
    {
        FVector Velocity = PlayerChar->GetVelocity();
        float PredictionTime = FMath::Min(0.5f, GetDistanceToPlayer(Enemy) / 1000.0f);
        TargetLocation += Velocity * PredictionTime * Params.AggressionLevel;
    }
    
    // Move to predicted location
    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(TargetLocation);
    MoveRequest.SetAcceptanceRadius(Params.AttackRange * 0.8f);
    MoveRequest.SetUsePathfinding(true);
    
    UE_LOG(LogTemp, Warning, TEXT("%s: MoveTo called - Target: %s, Location: %s, AcceptRadius: %.0f"),
        *Enemy->GetName(),
        *Target->GetName(),
        *TargetLocation.ToString(),
        Params.AttackRange * 0.8f);
    
    FPathFollowingRequestResult Result = AIController->MoveTo(MoveRequest);
    
    UE_LOG(LogTemp, Warning, TEXT("%s: MoveTo result: %s"),
        *Enemy->GetName(),
        Result.Code == EPathFollowingRequestResult::RequestSuccessful ? TEXT("Success") :
        Result.Code == EPathFollowingRequestResult::AlreadyAtGoal ? TEXT("Already at goal") :
        Result.Code == EPathFollowingRequestResult::Failed ? TEXT("Failed") : TEXT("Unknown"));
    
    // Additional debug for failed movement
    if (Result.Code == EPathFollowingRequestResult::Failed)
    {
        // Check if we have a valid navigation system
        UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
        if (!NavSystem)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: No navigation system found!"), *Enemy->GetName());
        }
        else
        {
            // Test if the target location is on navmesh
            FNavLocation NavLocation;
            bool bOnNavMesh = NavSystem->ProjectPointToNavigation(
                TargetLocation,
                NavLocation,
                FVector(500.0f, 500.0f, 500.0f) // Search extent
            );
            
            UE_LOG(LogTemp, Warning, TEXT("%s: Target on NavMesh: %s"), 
                *Enemy->GetName(), 
                bOnNavMesh ? TEXT("Yes") : TEXT("No"));
        }
    }
    
    LastPathUpdateTime = TimeInState;
}

bool UChaseState::ShouldRetreat(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const
{
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    float HealthPercent = GetHealthPercent(Enemy);
    
    // Retreat if health below threshold
    return HealthPercent <= Params.RetreatHealthPercent;
}

void UChaseState::OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage)
{
    // Check if we should retreat after taking damage
    if (ShouldRetreat(Enemy, StateMachine))
    {
        StateMachine->ChangeState(EEnemyState::Retreat);
    }
}