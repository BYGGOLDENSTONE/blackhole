#include "Enemy/AI/States/StandardCombatState.h"
#include "Enemy/StandardEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/SwordAttackComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/Abilities/Enemy/BuilderComponent.h"

void UStandardCombatState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    bHasCheckedBuilding = false;
    TimeSinceLastBuildCheck = 0.0f;
    
    // Check for building opportunity when entering combat
    if (AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(Enemy))
    {
        StandardEnemy->OnAlerted();
    }
}

void UStandardCombatState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    // Periodically check for building opportunities
    TimeSinceLastBuildCheck += DeltaTime;
    if (TimeSinceLastBuildCheck >= 5.0f)
    {
        TimeSinceLastBuildCheck = 0.0f;
        CheckBuildingOpportunity(Enemy);
    }
}

void UStandardCombatState::InitializeCombatActions(ABaseEnemy* Enemy)
{
    // Standard enemy actions: Sword attacks and defensive blocks
    AddCombatAction(TEXT("SwordAttack"), 3.0f, 1.5f, 0.0f, 180.0f);  // Primary sword attack
    AddCombatAction(TEXT("Block"), 2.0f, 1.0f, 0.0f, 300.0f);        // Defensive block
}

void UStandardCombatState::ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName)
{
    if (!Enemy) return;
    
    AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(Enemy);
    if (!StandardEnemy) return;
    
    if (ActionName == TEXT("SwordAttack"))
    {
        ExecuteSwordAttack(StandardEnemy);
        StartAbilityCooldown(Enemy, TEXT("SwordAttack"), 1.5f);
    }
    else if (ActionName == TEXT("Block"))
    {
        ExecuteBlock(StandardEnemy);
        StartAbilityCooldown(Enemy, TEXT("Block"), 1.0f);
    }
}

void UStandardCombatState::ExecuteSwordAttack(ABaseEnemy* Enemy)
{
    AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(Enemy);
    if (!StandardEnemy) return;
    
    if (USwordAttackComponent* SwordAttack = StandardEnemy->FindComponentByClass<USwordAttackComponent>())
    {
        SwordAttack->Execute();
        
        // Standard enemies maintain steady combat pace
        Enemy->ApplyMovementSpeedModifier(0.8f, 0.3f);
    }
}

void UStandardCombatState::ExecuteBlock(ABaseEnemy* Enemy)
{
    if (UBlockComponent* BlockAbility = Enemy->FindComponentByClass<UBlockComponent>())
    {
        BlockAbility->Execute();
        
        // Slight movement reduction while blocking
        Enemy->ApplyMovementSpeedModifier(0.6f, 1.0f);
    }
}

void UStandardCombatState::CheckBuildingOpportunity(ABaseEnemy* Enemy)
{
    AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(Enemy);
    if (!StandardEnemy) return;
    
    // If this enemy has builder component and isn't already building
    if (UBuilderComponent* BuilderComp = StandardEnemy->GetBuilderComponent())
    {
        if (!BuilderComp->IsBuilding())
        {
            // Call the standard enemy's building check
            StandardEnemy->OnAlerted();
        }
    }
}