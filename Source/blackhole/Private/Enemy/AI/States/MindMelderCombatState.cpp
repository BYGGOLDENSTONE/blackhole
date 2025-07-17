#include "Enemy/AI/States/MindMelderCombatState.h"
#include "Enemy/MindMelderEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/PowerfulMindmeldComponent.h"
#include "Components/Abilities/Enemy/DodgeComponent.h"
#include "AIController.h"

void UMindMelderCombatState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    bIsChanneling = false;
    TimeAtSafeDistance = 0.0f;
}

void UMindMelderCombatState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    if (!Enemy || !StateMachine || !StateMachine->GetTarget()) return;
    
    // Don't update normal combat behavior if channeling
    AMindMelderEnemy* MindMelder = Cast<AMindMelderEnemy>(Enemy);
    if (MindMelder && MindMelder->GetPowerfulMindmeld() && MindMelder->GetPowerfulMindmeld()->IsChanneling())
    {
        bIsChanneling = true;
        // Just wait for channeling to complete
        return;
    }
    else
    {
        bIsChanneling = false;
    }
    
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    // Always try to maintain safe distance
    MaintainSafeDistance(Enemy, StateMachine, DeltaTime);
    
    // Track time at safe distance - expanded range to match ability range
    float Distance = GetDistanceToPlayer(Enemy);
    if (Distance >= 1500.0f && Distance <= 3000.0f) // Match PowerfulMindmeld range
    {
        TimeAtSafeDistance += DeltaTime;
    }
    else
    {
        TimeAtSafeDistance = 0.0f;
    }
}

void UMindMelderCombatState::InitializeCombatActions(ABaseEnemy* Enemy)
{
    // Mind Melder actions: Powerful mindmeld and dodge
    AddCombatAction(TEXT("PowerfulMindmeld"), 10.0f, 45.0f, 1500.0f, 3000.0f);  // High priority, reduced cooldown
    AddCombatAction(TEXT("Dodge"), 3.0f, 2.0f, 0.0f, 500.0f);                   // Defensive dodge when close
}

void UMindMelderCombatState::ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName)
{
    if (!Enemy) return;
    
    AMindMelderEnemy* MindMelder = Cast<AMindMelderEnemy>(Enemy);
    if (!MindMelder) return;
    
    if (ActionName == TEXT("PowerfulMindmeld"))
    {
        ExecutePowerfulMindmeld(MindMelder, StateMachine);
        StartAbilityCooldown(Enemy, TEXT("PowerfulMindmeld"), 60.0f);
    }
    else if (ActionName == TEXT("Dodge"))
    {
        ExecuteDodge(MindMelder);
        StartAbilityCooldown(Enemy, TEXT("Dodge"), 2.0f);
    }
}

void UMindMelderCombatState::ExecutePowerfulMindmeld(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    AMindMelderEnemy* MindMelder = Cast<AMindMelderEnemy>(Enemy);
    if (!MindMelder) return;
    
    if (UPowerfulMindmeldComponent* Mindmeld = MindMelder->GetPowerfulMindmeld())
    {
        // Execute immediately when in range (removed time requirement)
        Mindmeld->Execute();
        
        // Mind melder should transition to channeling state
        if (Mindmeld->IsChanneling())
        {
            StateMachine->ChangeState(EEnemyState::Channeling);
            UE_LOG(LogTemp, Warning, TEXT("MindMelder: Started PowerfulMindmeld channel!"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MindMelder: Failed to start PowerfulMindmeld"));
        }
    }
}

void UMindMelderCombatState::ExecuteDodge(ABaseEnemy* Enemy)
{
    if (UDodgeComponent* DodgeAbility = Enemy->FindComponentByClass<UDodgeComponent>())
    {
        DodgeAbility->Execute();
    }
}

void UMindMelderCombatState::MaintainSafeDistance(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    if (!Enemy || !StateMachine->GetTarget()) return;
    
    float Distance = GetDistanceToPlayer(Enemy);
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    
    // If too close, move away
    if (Distance < Params.PreferredCombatDistance * 0.9f)
    {
        // Calculate direction away from player
        FVector DirectionAway = (Enemy->GetActorLocation() - StateMachine->GetTarget()->GetActorLocation()).GetSafeNormal();
        FVector DesiredLocation = Enemy->GetActorLocation() + DirectionAway * 300.0f;
        
        // Use AI movement
        if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
        {
            AIController->MoveToLocation(DesiredLocation, 10.0f, false);
        }
    }
    // If too far, move slightly closer
    else if (Distance > Params.PreferredCombatDistance * 1.1f)
    {
        // Move towards player but stop at preferred distance
        if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
        {
            FVector DirectionToTarget = (StateMachine->GetTarget()->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
            FVector DesiredLocation = Enemy->GetActorLocation() + DirectionToTarget * 200.0f;
            AIController->MoveToLocation(DesiredLocation, 10.0f, false);
        }
    }
}