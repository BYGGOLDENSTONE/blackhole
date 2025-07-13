#include "Enemy/AI/States/ChannelingState.h"
#include "Enemy/HackerEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/MindmeldComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

void UChannelingState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    if (!Enemy) return;
    
    ChannelDuration = 0.0f;
    bChannelInterrupted = false;
    
    // Stop movement while channeling
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }
    
    StartChanneling(Enemy);
}

void UChannelingState::Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Exit(Enemy, StateMachine);
    
    StopChanneling(Enemy);
    
    // Re-enable movement
    if (Enemy && Enemy->GetCharacterMovement())
    {
        Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
}

void UChannelingState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!Enemy || !StateMachine || bChannelInterrupted) return;
    
    ChannelDuration += DeltaTime;
    
    // Check if we still have line of sight
    if (!HasLineOfSightToPlayer(Enemy))
    {
        bChannelInterrupted = true;
        StateMachine->ChangeState(EEnemyState::Alert);
        return;
    }
    
    // Check if target moved out of range
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    if (GetDistanceToPlayer(Enemy) > Params.AttackRange * 2.0f)
    {
        bChannelInterrupted = true;
        StateMachine->ChangeState(EEnemyState::Chase);
        return;
    }
    
    // Check if channeled long enough
    if (ChannelDuration >= MaxChannelTime)
    {
        // Successful channel completion - go on cooldown
        StartAbilityCooldown(Enemy, TEXT("Mindmeld"), 5.0f);
        StateMachine->ChangeState(EEnemyState::Combat);
        return;
    }
    
    // Visual feedback - draw beam to target
    #if WITH_EDITOR
    if (StateMachine->GetTarget())
    {
        FVector Start = Enemy->GetActorLocation() + FVector(0, 0, 50);
        FVector End = StateMachine->GetTarget()->GetActorLocation();
        DrawDebugLine(Enemy->GetWorld(), Start, End, FColor::Purple, false, 0.1f, 0, 2.0f);
    }
    #endif
}

void UChannelingState::StartChanneling(ABaseEnemy* Enemy)
{
    AHackerEnemy* Hacker = Cast<AHackerEnemy>(Enemy);
    if (!Hacker) return;
    
    if (UMindmeldComponent* Mindmeld = Hacker->FindComponentByClass<UMindmeldComponent>())
    {
        Mindmeld->Execute();
        UE_LOG(LogTemp, Warning, TEXT("%s: Started channeling Mindmeld"), *Enemy->GetName());
    }
}

void UChannelingState::StopChanneling(ABaseEnemy* Enemy)
{
    AHackerEnemy* Hacker = Cast<AHackerEnemy>(Enemy);
    if (!Hacker) return;
    
    if (UMindmeldComponent* Mindmeld = Hacker->FindComponentByClass<UMindmeldComponent>())
    {
        // Stop the channel - this would need to be implemented in MindmeldAbility
        // For now, just log it
        UE_LOG(LogTemp, Warning, TEXT("%s: Stopped channeling Mindmeld (Duration: %.2fs)"), 
            *Enemy->GetName(), ChannelDuration);
    }
}

void UChannelingState::OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage)
{
    // Taking damage interrupts channeling
    bChannelInterrupted = true;
    
    // Put ability on shorter cooldown since it was interrupted
    StartAbilityCooldown(Enemy, TEXT("Mindmeld"), 2.0f);
    
    StateMachine->ChangeState(EEnemyState::Combat);
}

bool UChannelingState::CanTransitionTo(EEnemyState NewState) const
{
    // Can only transition if interrupted or via the Update logic
    return bChannelInterrupted || NewState == EEnemyState::Dead;
}