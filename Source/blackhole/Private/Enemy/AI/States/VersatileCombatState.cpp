#include "Enemy/AI/States/VersatileCombatState.h"
#include "Enemy/CombatEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/Abilities/Enemy/DodgeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UVersatileCombatState::Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Exit(Enemy, StateMachine);
    
    // Clear all combo timers
    if (Enemy && Enemy->GetWorld())
    {
        for (FTimerHandle& Handle : ComboTimerHandles)
        {
            Enemy->GetWorld()->GetTimerManager().ClearTimer(Handle);
        }
        ComboTimerHandles.Empty();
    }
    
    // Reset combo state
    bIsInCombo = false;
    ComboHitCount = 0;
}

void UVersatileCombatState::InitializeCombatActions(ABaseEnemy* Enemy)
{
    // Combat drone has access to all actions
    AddCombatAction(TEXT("Smash"), 2.5f, 2.0f, 0.0f, 300.0f);      // Standard attack
    AddCombatAction(TEXT("Block"), 2.0f, 1.5f, 0.0f, 400.0f);      // Defensive
    AddCombatAction(TEXT("Dodge"), 2.0f, 1.0f, 0.0f, 350.0f);      // Evasive
    AddCombatAction(TEXT("Combo"), 1.5f, 3.0f, 0.0f, 250.0f);      // Multi-hit combo
}

void UVersatileCombatState::ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName)
{
    if (!Enemy) return;
    
    ACombatEnemy* Combat = Cast<ACombatEnemy>(Enemy);
    if (!Combat) return;
    
    if (ActionName == TEXT("Smash"))
    {
        ExecuteSmashAttack(Combat);
        StartAbilityCooldown(Enemy, TEXT("Smash"), 2.0f);
    }
    else if (ActionName == TEXT("Block"))
    {
        ExecuteBlockDefense(Combat);
        StartAbilityCooldown(Enemy, TEXT("Block"), 1.5f);
    }
    else if (ActionName == TEXT("Dodge"))
    {
        ExecuteDodgeRoll(Combat, StateMachine);
        StartAbilityCooldown(Enemy, TEXT("Dodge"), 1.0f);
    }
    else if (ActionName == TEXT("Combo"))
    {
        ExecuteComboStrike(Combat);
        StartAbilityCooldown(Enemy, TEXT("Combo"), 3.0f);
    }
}

void UVersatileCombatState::ExecuteSmashAttack(ABaseEnemy* Enemy)
{
    if (USmashAbilityComponent* SmashAbility = Enemy->FindComponentByClass<USmashAbilityComponent>())
    {
        SmashAbility->Execute();
        
        // Standard recovery
        Enemy->ApplyMovementSpeedModifier(0.7f, 0.5f);
    }
}

void UVersatileCombatState::ExecuteBlockDefense(ABaseEnemy* Enemy)
{
    if (UBlockComponent* BlockAbility = Enemy->FindComponentByClass<UBlockComponent>())
    {
        BlockAbility->Execute();
        
        // Can move slowly while blocking
        Enemy->ApplyMovementSpeedModifier(0.3f, 1.0f);
    }
}

void UVersatileCombatState::ExecuteDodgeRoll(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (UDodgeComponent* DodgeAbility = Enemy->FindComponentByClass<UDodgeComponent>())
    {
        DodgeAbility->Execute();
        
        // Dodge direction based on player position
        if (StateMachine && StateMachine->GetTarget())
        {
            FVector ToTarget = (StateMachine->GetTarget()->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
            FVector DodgeDirection = FVector::CrossProduct(ToTarget, FVector::UpVector);
            
            // Alternate dodge direction
            static bool bDodgeLeft = true;
            if (!bDodgeLeft)
            {
                DodgeDirection = -DodgeDirection;
            }
            bDodgeLeft = !bDodgeLeft;
            
            // Apply dodge movement
            if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
            {
                Movement->AddImpulse(DodgeDirection * 800.0f, true);
            }
        }
    }
}

void UVersatileCombatState::ExecuteComboStrike(ABaseEnemy* Enemy)
{
    if (!Enemy || !Enemy->GetWorld()) return;
    
    // Execute a 3-hit combo
    bIsInCombo = true;
    ComboHitCount = 0;
    
    // Clear any existing combo timers
    for (FTimerHandle& Handle : ComboTimerHandles)
    {
        Enemy->GetWorld()->GetTimerManager().ClearTimer(Handle);
    }
    ComboTimerHandles.Empty();
    
    // Schedule combo hits with proper timer management
    TWeakObjectPtr<ABaseEnemy> WeakEnemy = Enemy;
    for (int32 i = 0; i < 3; i++)
    {
        FTimerHandle NewTimer;
        float HitMultiplier = 1.0f + 0.2f * i; // Store multiplier to avoid capture issues
        
        Enemy->GetWorld()->GetTimerManager().SetTimer(NewTimer, [WeakEnemy, HitMultiplier]()
        {
            if (WeakEnemy.IsValid())
            {
                if (USmashAbilityComponent* SmashAbility = WeakEnemy->FindComponentByClass<USmashAbilityComponent>())
                {
                    float OriginalDamage = SmashAbility->GetDamage();
                    SmashAbility->SetDamage(OriginalDamage * HitMultiplier);
                    SmashAbility->Execute();
                    SmashAbility->SetDamage(OriginalDamage);
                }
            }
        }, 0.4f * (i + 1), false);
        
        ComboTimerHandles.Add(NewTimer);
    }
    
    // Movement penalty during combo
    Enemy->ApplyMovementSpeedModifier(0.5f, 1.5f);
}

void UVersatileCombatState::OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    // Versatile response - might block or dodge
    if (!Enemy || !StateMachine) return;
    
    float RandomChoice = FMath::RandRange(0.0f, 1.0f);
    
    if (RandomChoice < 0.4f && !IsAbilityOnCooldown(Enemy, TEXT("Dodge")))
    {
        ExecuteDodgeRoll(Enemy, StateMachine);
        StartAbilityCooldown(Enemy, TEXT("Dodge"), 1.0f);
    }
    else if (RandomChoice < 0.7f && !IsAbilityOnCooldown(Enemy, TEXT("Block")))
    {
        ExecuteBlockDefense(Enemy);
        StartAbilityCooldown(Enemy, TEXT("Block"), 1.5f);
    }
}

void UVersatileCombatState::OnPlayerAttacking(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    // High chance to defend when player attacks
    if (!Enemy || !StateMachine) return;
    
    if (FMath::RandRange(0.0f, 1.0f) < 0.6f && !IsAbilityOnCooldown(Enemy, TEXT("Block")))
    {
        ExecuteBlockDefense(Enemy);
        StartAbilityCooldown(Enemy, TEXT("Block"), 1.5f);
    }
}