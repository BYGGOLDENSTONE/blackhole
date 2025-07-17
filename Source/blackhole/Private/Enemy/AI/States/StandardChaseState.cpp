#include "Enemy/AI/States/StandardChaseState.h"
#include "Enemy/StandardEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/BuilderComponent.h"

void UStandardChaseState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    // Reset building timer when entering chase
    TimeSinceChaseStart = 0.0f;
    bHasTriggeredBuilding = false;
    
    UE_LOG(LogTemp, Warning, TEXT("%s: Entering chase state, will build after %.1f seconds if can't reach player"), 
        *Enemy->GetName(), BuildAfterChaseTime);
}

void UStandardChaseState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!Enemy || !StateMachine) return;
    
    // Track time spent chasing
    TimeSinceChaseStart += DeltaTime;
    
    // Check if we should start building
    if (!bHasTriggeredBuilding && TimeSinceChaseStart >= BuildAfterChaseTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Can't reach player after %.1f seconds of chasing, checking building opportunity!"), 
            *Enemy->GetName(), BuildAfterChaseTime);
        
        CheckBuildingOpportunity(Enemy);
        bHasTriggeredBuilding = true; // Only trigger once per chase
    }
}

void UStandardChaseState::CheckBuildingOpportunity(ABaseEnemy* Enemy)
{
    AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(Enemy);
    if (!StandardEnemy) return;
    
    // If this enemy has builder component and isn't already building
    if (UBuilderComponent* BuilderComp = StandardEnemy->GetBuilderComponent())
    {
        if (!BuilderComp->IsBuilding())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Initiating Psi-Disruptor build after chase timeout!"), *Enemy->GetName());
            
            // Call the standard enemy's building check which coordinates with nearby builders
            StandardEnemy->OnAlerted();
        }
    }
}