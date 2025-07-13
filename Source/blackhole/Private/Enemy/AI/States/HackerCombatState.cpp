#include "Enemy/AI/States/HackerCombatState.h"
#include "Enemy/HackerEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/MindmeldComponent.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"

void UHackerCombatState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    // Override update to check for mindmeld opportunities
    if (!Enemy || !StateMachine) return;
    
    // Check if we should start channeling mindmeld
    float Distance = GetDistanceToPlayer(Enemy);
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    
    if (Distance >= 500.0f && Distance <= 1200.0f && 
        !IsAbilityOnCooldown(Enemy, TEXT("Mindmeld")) &&
        HasLineOfSightToPlayer(Enemy))
    {
        // Perfect range for mindmeld - switch to channeling state
        StateMachine->ChangeState(EEnemyState::Channeling);
        return;
    }
    
    // Otherwise continue normal combat behavior
    Super::Update(Enemy, StateMachine, DeltaTime);
}

void UHackerCombatState::InitializeCombatActions(ABaseEnemy* Enemy)
{
    // Hacker actions: Ranged attacks and repositioning
    AddCombatAction(TEXT("PulseHack"), 3.0f, 3.0f, 0.0f, 600.0f);         // AoE slow
    AddCombatAction(TEXT("Mindmeld"), 2.0f, 5.0f, 500.0f, 1200.0f);       // WP drain
    AddCombatAction(TEXT("Reposition"), 2.0f, 2.0f, 0.0f, 400.0f);        // Maintain distance
}

void UHackerCombatState::ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName)
{
    if (!Enemy) return;
    
    AHackerEnemy* Hacker = Cast<AHackerEnemy>(Enemy);
    if (!Hacker) return;
    
    if (ActionName == TEXT("PulseHack"))
    {
        ExecutePulseHack(Hacker);
        StartAbilityCooldown(Enemy, TEXT("PulseHack"), 3.0f);
    }
    else if (ActionName == TEXT("Mindmeld"))
    {
        ExecuteMindmeldChannel(Hacker, StateMachine);
        // Cooldown handled by channeling state
    }
    else if (ActionName == TEXT("Reposition"))
    {
        ExecuteReposition(Hacker, StateMachine);
        StartAbilityCooldown(Enemy, TEXT("Reposition"), 2.0f);
    }
}

void UHackerCombatState::ExecutePulseHack(ABaseEnemy* Enemy)
{
    // Pulse Hack - AoE slow effect (using smash ability as base)
    if (USmashAbilityComponent* SmashAbility = Enemy->FindComponentByClass<USmashAbilityComponent>())
    {
        // Configure for AoE slow instead of damage
        float OriginalDamage = SmashAbility->GetDamage();
        SmashAbility->SetDamage(5.0f); // Minimal damage
        SmashAbility->Execute();
        SmashAbility->SetDamage(OriginalDamage);
        
        // TODO: Apply slow effect to hit targets
        UE_LOG(LogTemp, Warning, TEXT("%s: Executed Pulse Hack (AoE slow)"), *Enemy->GetName());
    }
}

void UHackerCombatState::ExecuteMindmeldChannel(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    // Transition to channeling state for mindmeld
    if (StateMachine)
    {
        StateMachine->ChangeState(EEnemyState::Channeling);
    }
}

void UHackerCombatState::ExecuteReposition(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (!Enemy || !StateMachine || !StateMachine->GetTarget()) return;
    
    AAIController* AIController = Cast<AAIController>(Enemy->GetController());
    if (!AIController) return;
    
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    float CurrentDistance = GetDistanceToPlayer(Enemy);
    FVector EnemyLocation = Enemy->GetActorLocation();
    FVector TargetLocation = StateMachine->GetTarget()->GetActorLocation();
    FVector ToTarget = (TargetLocation - EnemyLocation).GetSafeNormal();
    
    FVector DesiredLocation;
    
    if (CurrentDistance < 600.0f)
    {
        // Too close - back away
        DesiredLocation = EnemyLocation - ToTarget * 400.0f;
    }
    else
    {
        // Strafe to maintain distance
        FVector RightVector = FVector::CrossProduct(ToTarget, FVector::UpVector);
        float StrafeDirection = FMath::RandBool() ? 1.0f : -1.0f;
        DesiredLocation = EnemyLocation + RightVector * StrafeDirection * 300.0f;
    }
    
    // Find navigable point
    FNavLocation NavLocation;
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
    
    if (NavSystem && NavSystem->GetRandomReachablePointInRadius(DesiredLocation, 200.0f, NavLocation))
    {
        AIController->MoveToLocation(NavLocation.Location, 50.0f);
    }
}

bool UHackerCombatState::ShouldMaintainDistance(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const
{
    // Hackers always try to maintain optimal distance for their abilities
    float Distance = GetDistanceToPlayer(Enemy);
    return Distance < 500.0f || Distance > 1200.0f;
}