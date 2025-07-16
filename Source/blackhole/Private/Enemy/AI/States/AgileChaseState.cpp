#include "Enemy/AI/States/AgileChaseState.h"
#include "Enemy/AgileEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Engine/Engine.h"

void UAgileChaseState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    // Don't call parent - we handle everything custom
    TimeInState = 0.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("Agile Enemy: Entering CUSTOM chase state - will maintain distance"));
}

void UAgileChaseState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    TimeInState += DeltaTime;
    
    if (!Enemy || !StateMachine || !StateMachine->GetTarget()) return;
    
    AActor* Target = StateMachine->GetTarget();
    float DistanceToTarget = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
    
    // Check if we should transition to combat (when dash is ready AND in range)
    bool bDashOnCooldown = IsAbilityOnCooldown(Enemy, TEXT("DashAttack"));
    
    // Only transition to combat when dash is ready AND we're in dash range
    if (!bDashOnCooldown && DistanceToTarget <= 600.0f) // Increased range for more aggressive behavior
    {
        StateMachine->ChangeState(EEnemyState::Combat);
        return;
    }
    
    // If player is far away, chase them normally
    if (DistanceToTarget > MaxDistance)
    {
        // Normal chase behavior to close the gap
        AAIController* AIController = Cast<AAIController>(Enemy->GetController());
        if (AIController)
        {
            AIController->MoveToActor(Target, MinDistance);
        }
    }
    else
    {
        // We're in preferred range - maintain distance
        MaintainDistance(Enemy, StateMachine, DeltaTime);
    }
    
    // Show debug info
    if (GEngine)
    {
        float DashCooldownRemaining = StateMachine->GetCooldownRemaining(TEXT("DashAttack"));
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, 
            FString::Printf(TEXT("Agile Chase: Dist: %.0f | Assassin Approach CD: %.1fs"), 
                DistanceToTarget, DashCooldownRemaining));
    }
}

void UAgileChaseState::MaintainDistance(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    if (!Enemy || !StateMachine || !StateMachine->GetTarget()) return;
    
    AActor* Target = StateMachine->GetTarget();
    AAIController* AIController = Cast<AAIController>(Enemy->GetController());
    if (!AIController) return;
    
    float CurrentDistance = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
    FVector EnemyLocation = Enemy->GetActorLocation();
    FVector TargetLocation = Target->GetActorLocation();
    
    if (CurrentDistance < MinDistance)
    {
        // Too close - move away
        FVector DirectionAway = (EnemyLocation - TargetLocation).GetSafeNormal();
        FVector RetreatPosition = EnemyLocation + (DirectionAway * 200.0f);
        
        // Make sure retreat position is navigable
        if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Enemy->GetWorld()))
        {
            FNavLocation NavLocation;
            if (NavSys->GetRandomReachablePointInRadius(RetreatPosition, 100.0f, NavLocation))
            {
                AIController->MoveToLocation(NavLocation.Location, 50.0f);
            }
        }
    }
    else if (CurrentDistance > MaxDistance)
    {
        // Too far - move closer but not too close
        AIController->MoveToActor(Target, PreferredDistance);
    }
    else
    {
        // Good distance - circle strafe to stay mobile
        FVector ToTarget = (TargetLocation - EnemyLocation).GetSafeNormal();
        FVector StrafeDirection = FVector::CrossProduct(ToTarget, FVector::UpVector);
        
        // Randomly pick left or right
        static float StrafeTimer = 0.0f;
        static float StrafeDirection_Sign = 1.0f;
        StrafeTimer += DeltaTime;
        
        if (StrafeTimer > 2.0f)
        {
            StrafeDirection_Sign *= -1.0f;
            StrafeTimer = 0.0f;
        }
        
        FVector StrafePosition = EnemyLocation + (StrafeDirection * StrafeDirection_Sign * 150.0f);
        AIController->MoveToLocation(StrafePosition, 10.0f);
        
        // Face the target
        FRotator LookAtRotation = ToTarget.Rotation();
        Enemy->SetActorRotation(FRotator(0.0f, LookAtRotation.Yaw, 0.0f));
    }
}