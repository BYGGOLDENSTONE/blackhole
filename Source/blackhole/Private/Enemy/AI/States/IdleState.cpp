#include "Enemy/AI/States/IdleState.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

void UIdleState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    if (!Enemy) return;
    
    InitialLocation = Enemy->GetActorLocation();
    NextWanderTime = FMath::RandRange(3.0f, 6.0f);
    
    // Stop movement
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
}

void UIdleState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!Enemy || !StateMachine) return;
    
    // Debug: Log update status
    static int UpdateCounter = 0;
    if (UpdateCounter++ % 30 == 0) // Every 3 seconds
    {
        UE_LOG(LogTemp, Warning, TEXT("%s IdleState: Update called - TimeInState: %.1f"),
            *Enemy->GetName(), TimeInState);
    }
    
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    
    // Debug logging
    AActor* Target = StateMachine->GetTarget();
    if (!Target && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, 
            FString::Printf(TEXT("%s: No target set in StateMachine!"), *Enemy->GetName()));
    }
    
    // Check for player in sight range
    if (Target && IsPlayerInRange(Enemy, Params.SightRange))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s IdleState: Player in range! Checking LOS..."), *Enemy->GetName());
        
        if (HasLineOfSightToPlayer(Enemy))
        {
            // Player spotted - transition to alert
            UE_LOG(LogTemp, Warning, TEXT("%s: Player spotted! Transitioning to Alert state"), *Enemy->GetName());
            StateMachine->UpdateLastKnownTargetLocation();
            StateMachine->ChangeState(EEnemyState::Alert);
            return;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s IdleState: Player in range but NO line of sight"), *Enemy->GetName());
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, 
                    FString::Printf(TEXT("%s: Player in range but no line of sight"), *Enemy->GetName()));
            }
        }
    }
    else
    {
        static int NoTargetCounter = 0;
        if (NoTargetCounter++ % 30 == 0) // Every 3 seconds
        {
            UE_LOG(LogTemp, Warning, TEXT("%s IdleState: No target in range (Target: %s, SightRange: %.0f)"),
                *Enemy->GetName(), 
                Target ? *Target->GetName() : TEXT("NULL"),
                Params.SightRange);
        }
    }
    
    // Debug: Draw detection sphere
    if (Enemy->GetWorld())
    {
        DrawDebugSphere(Enemy->GetWorld(), Enemy->GetActorLocation(), Params.SightRange, 24, FColor::Blue, false, 0.1f, 0, 1.0f);
    }
    
    // Idle wander behavior
    if (TimeInState >= NextWanderTime)
    {
        // Small random movement within idle area
        FNavLocation RandomLocation;
        UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
        
        if (NavSystem && NavSystem->GetRandomPointInNavigableRadius(InitialLocation, IdleWanderRadius, RandomLocation))
        {
            if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
            {
                AIController->MoveToLocation(RandomLocation.Location, 1.0f);
            }
        }
        
        NextWanderTime = TimeInState + FMath::RandRange(3.0f, 6.0f);
    }
}