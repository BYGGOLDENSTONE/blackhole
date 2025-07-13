#include "Enemy/AI/States/TankCombatState.h"
#include "Enemy/TankEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"

void UTankCombatState::InitializeCombatActions(ABaseEnemy* Enemy)
{
    // Tank actions: Heavy attacks and defensive blocks
    AddCombatAction(TEXT("Smash"), 3.0f, 2.5f, 0.0f, 250.0f);      // Primary attack - matches SmashAbility range
    AddCombatAction(TEXT("Block"), 2.0f, 1.5f, 0.0f, 500.0f);      // Defensive stance
    AddCombatAction(TEXT("GroundSlam"), 1.0f, 5.0f, 0.0f, 300.0f); // AoE attack - matches area radius
}

void UTankCombatState::ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName)
{
    if (!Enemy) return;
    
    ATankEnemy* Tank = Cast<ATankEnemy>(Enemy);
    if (!Tank) return;
    
    if (ActionName == TEXT("Smash"))
    {
        ExecuteSmashAttack(Tank);
        StartAbilityCooldown(Enemy, TEXT("Smash"), 2.5f);
    }
    else if (ActionName == TEXT("Block"))
    {
        ExecuteBlockStance(Tank);
        StartAbilityCooldown(Enemy, TEXT("Block"), 1.5f);
    }
    else if (ActionName == TEXT("GroundSlam"))
    {
        ExecuteGroundSlam(Tank);
        StartAbilityCooldown(Enemy, TEXT("GroundSlam"), 5.0f);
    }
}

void UTankCombatState::ExecuteSmashAttack(ABaseEnemy* Enemy)
{
    if (USmashAbilityComponent* SmashAbility = Enemy->FindComponentByClass<USmashAbilityComponent>())
    {
        SmashAbility->Execute();
        
        // Tank specific: Slow movement after heavy attack
        Enemy->ApplyMovementSpeedModifier(0.5f, 0.5f);
    }
}

void UTankCombatState::ExecuteBlockStance(ABaseEnemy* Enemy)
{
    if (UBlockComponent* BlockAbility = Enemy->FindComponentByClass<UBlockComponent>())
    {
        BlockAbility->Execute();
        
        // Tank specific: Can't move while blocking
        Enemy->ApplyMovementSpeedModifier(0.0f, 1.5f);
    }
}

void UTankCombatState::ExecuteGroundSlam(ABaseEnemy* Enemy)
{
    // Tank special ability - AoE ground slam
    ATankEnemy* Tank = Cast<ATankEnemy>(Enemy);
    if (!Tank) return;
    
    if (USmashAbilityComponent* SmashAbility = Enemy->FindComponentByClass<USmashAbilityComponent>())
    {
        // Store original values
        float OriginalDamage = SmashAbility->GetDamage();
        bool OriginalAreaDamage = SmashAbility->bIsAreaDamage;
        float OriginalRadius = SmashAbility->GetAreaRadius();
        float OriginalKnockback = SmashAbility->GetKnockbackForce();
        
        // Apply tank's ground slam configuration
        SmashAbility->SetDamage(OriginalDamage * Tank->GroundSlamDamageMultiplier);
        SmashAbility->SetAreaDamage(true);  // Enable area damage for ground slam
        SmashAbility->SetAreaRadius(Tank->GroundSlamRadius);
        SmashAbility->SetKnockbackForce(Tank->GroundSlamKnockbackForce);
        SmashAbility->Execute();
        
        // Reset to original values
        SmashAbility->SetDamage(OriginalDamage);
        SmashAbility->SetAreaDamage(OriginalAreaDamage);
        SmashAbility->SetAreaRadius(OriginalRadius);
        SmashAbility->SetKnockbackForce(OriginalKnockback);
        
        // Longer recovery for powerful attack - tank staggers after ground slam
        Enemy->ApplyMovementSpeedModifier(0.0f, 1.5f);  // Complete stop for 1.5 seconds
        
        // Additional stagger effect - prevent all actions during recovery
        StartAbilityCooldown(Enemy, TEXT("Smash"), 2.0f);  // Block other attacks during stagger
        StartAbilityCooldown(Enemy, TEXT("Block"), 2.0f);
        
        UE_LOG(LogTemp, Warning, TEXT("Tank GroundSlam: Executed area damage attack - Staggered for 1.5s"));
    }
}