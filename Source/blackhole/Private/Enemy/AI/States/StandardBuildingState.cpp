#include "Enemy/AI/States/StandardBuildingState.h"
#include "Enemy/StandardEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/BuilderComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"

UStandardBuildingState::UStandardBuildingState()
{
    StayInRangeDistance = 400.0f;
    MaxTimeOutOfRange = 2.0f;
}

void UStandardBuildingState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    if (!Enemy) return;
    
    // Get builder component
    if (AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(Enemy))
    {
        CachedBuilderComponent = StandardEnemy->GetBuilderComponent();
        if (CachedBuilderComponent && CachedBuilderComponent->IsBuilding())
        {
            BuildLocation = CachedBuilderComponent->GetBuildLocation();
            TimeOutOfRange = 0.0f;
            
            // Stop any current movement
            if (AAIController* AIController = Enemy->GetAIController())
            {
                AIController->StopMovement();
            }
            
            UE_LOG(LogTemp, Warning, TEXT("%s: Entering building state at location %s"), 
                *Enemy->GetName(), *BuildLocation.ToString());
        }
        else
        {
            // Not actually building, return to previous state
            StateMachine->ChangeState(EEnemyState::Idle);
        }
    }
}

void UStandardBuildingState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!Enemy || !StateMachine || !CachedBuilderComponent) return;
    
    // Check if still building
    if (!CachedBuilderComponent->IsBuilding())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Building complete or cancelled, returning to idle"), 
            *Enemy->GetName());
        StateMachine->ChangeState(EEnemyState::Idle);
        return;
    }
    
    // Check if paused
    if (CachedBuilderComponent->IsBuildPaused())
    {
        // Building is paused, we can return to combat/chase
        UE_LOG(LogTemp, Warning, TEXT("%s: Building paused, returning to combat"), 
            *Enemy->GetName());
        StateMachine->ChangeState(EEnemyState::Combat);
        return;
    }
    
    // Check distance to build location
    if (!IsInBuildRange(Enemy))
    {
        TimeOutOfRange += DeltaTime;
        
        if (TimeOutOfRange > MaxTimeOutOfRange)
        {
            // Too far from build location for too long, move back
            MoveTowardsBuildLocation(Enemy, DeltaTime);
        }
    }
    else
    {
        TimeOutOfRange = 0.0f;
        
        // In range, stop moving and face build location
        if (AAIController* AIController = Enemy->GetAIController())
        {
            AIController->StopMovement();
            
            // Face the build location
            FVector ToBuilding = BuildLocation - Enemy->GetActorLocation();
            ToBuilding.Z = 0.0f;
            if (ToBuilding.SizeSquared() > 0.0f)
            {
                FRotator LookRotation = ToBuilding.Rotation();
                Enemy->SetActorRotation(LookRotation);
            }
        }
    }
    
    // Visual feedback
    #if WITH_EDITOR
    FVector EnemyLoc = Enemy->GetActorLocation();
    DrawDebugLine(GetWorld(), EnemyLoc, BuildLocation, FColor::Blue, false, -1.0f, 0, 2.0f);
    DrawDebugCircle(GetWorld(), BuildLocation, StayInRangeDistance, 32, FColor::Blue, false, -1.0f, 0, 2.0f);
    #endif
}

void UStandardBuildingState::Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Exit(Enemy, StateMachine);
    
    CachedBuilderComponent = nullptr;
    
    UE_LOG(LogTemp, Warning, TEXT("%s: Exiting building state"), 
        Enemy ? *Enemy->GetName() : TEXT("NULL"));
}

void UStandardBuildingState::MoveTowardsBuildLocation(ABaseEnemy* Enemy, float DeltaTime)
{
    if (!Enemy) return;
    
    if (AAIController* AIController = Enemy->GetAIController())
    {
        // Move to a position within the build radius
        FVector Direction = BuildLocation - Enemy->GetActorLocation();
        Direction.Z = 0.0f;
        Direction.Normalize();
        
        FVector TargetLocation = BuildLocation - (Direction * (StayInRangeDistance * 0.8f));
        
        AIController->MoveToLocation(TargetLocation, 10.0f, true, true, false);
    }
}

bool UStandardBuildingState::IsInBuildRange(ABaseEnemy* Enemy) const
{
    if (!Enemy) return false;
    
    float Distance = FVector::Dist2D(Enemy->GetActorLocation(), BuildLocation);
    return Distance <= StayInRangeDistance;
}