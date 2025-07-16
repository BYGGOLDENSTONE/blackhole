#include "Enemy/AI/States/TankCombatState.h"
#include "Enemy/TankEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/Abilities/Enemy/AreaDamageAbilityComponent.h"

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
    ATankEnemy* Tank = Cast<ATankEnemy>(Enemy);
    if (!Tank) return;
    
    if (UAreaDamageAbilityComponent* AreaDamageAbility = Tank->GetAreaDamageAbility())
    {
        // Configure for normal smash attack (smaller radius)
        float OriginalRadius = AreaDamageAbility->DamageRadius;
        AreaDamageAbility->DamageRadius = 200.0f;  // Smaller radius for normal attack
        
        AreaDamageAbility->Execute();
        
        // Restore original radius
        AreaDamageAbility->DamageRadius = OriginalRadius;
        
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
    
    if (UAreaDamageAbilityComponent* AreaDamageAbility = Tank->GetAreaDamageAbility())
    {
        // The area damage ability is already configured for ground slam in TankEnemy constructor
        // Just execute it - it will handle knockback, damage, and self-stagger automatically
        AreaDamageAbility->Execute();
        
        // Additional stagger effect - prevent all actions during recovery
        StartAbilityCooldown(Enemy, TEXT("Smash"), 2.0f);  // Block other attacks during stagger
        StartAbilityCooldown(Enemy, TEXT("Block"), 2.0f);
        
        UE_LOG(LogTemp, Warning, TEXT("Tank GroundSlam: Executed area damage attack - Staggered for 1.5s"));
    }
}