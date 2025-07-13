#include "Enemy/AI/States/CombatState.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

void UCombatState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    if (!Enemy) return;
    
    // Initialize combat actions for this enemy type
    InitializeCombatActions(Enemy);
    
    // Set combat speed (slightly slower for tactical movement)
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = Enemy->GetDefaultWalkSpeed() * 0.8f;
    }
    
    NextActionTime = TimeInState + StateMachine->GetAIParameters().ReactionTime;
    bIsExecutingAction = false;
}

void UCombatState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!Enemy || !StateMachine || !StateMachine->GetTarget()) return;
    
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    float Distance = GetDistanceToPlayer(Enemy);
    
    // Check state transitions
    if (GetHealthPercent(Enemy) <= Params.RetreatHealthPercent)
    {
        StateMachine->ChangeState(EEnemyState::Retreat);
        return;
    }
    
    if (Distance > Params.AttackRange * 1.5f)
    {
        StateMachine->ChangeState(EEnemyState::Chase);
        return;
    }
    
    if (!HasLineOfSightToPlayer(Enemy))
    {
        StateMachine->ChangeState(EEnemyState::Alert);
        return;
    }
    
    // Check if stuck in combat too long
    if (TimeInState > Params.MaxTimeInCombat && TimeInState - LastAttackTime > 3.0f)
    {
        // Haven't attacked in a while - reposition
        StateMachine->ChangeState(EEnemyState::Chase);
        return;
    }
    
    // Execute combat behavior
    if (!bIsExecutingAction && TimeInState >= NextActionTime)
    {
        FString SelectedAction = SelectCombatAction(Enemy, StateMachine);
        if (!SelectedAction.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s CombatState: Executing action %s at distance %.0f"), 
                *Enemy->GetName(), *SelectedAction, Distance);
                
            bIsExecutingAction = true;
            ExecuteCombatAction(Enemy, StateMachine, SelectedAction);
            bIsExecutingAction = false;
            LastAttackTime = TimeInState;
            
            // Update action cooldown
            if (FCombatAction* Action = CombatActions.Find(SelectedAction))
            {
                Action->LastUsedTime = TimeInState;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s CombatState: No valid action at distance %.0f"), 
                *Enemy->GetName(), Distance);
        }
        
        // Schedule next action - more aggressive timing
        NextActionTime = TimeInState + FMath::RandRange(Params.AttackCooldown * 0.5f, Params.AttackCooldown * 0.8f);
    }
    
    // Update position when not attacking
    if (!bIsExecutingAction)
    {
        UpdateCombatPosition(Enemy, StateMachine);
    }
    
    // Always rotate to face target in combat
    RotateTowardsTarget(Enemy, StateMachine, DeltaTime, 8.0f); // Faster rotation in combat
}

FString UCombatState::SelectCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    float Distance = GetDistanceToPlayer(Enemy);
    float CurrentTime = TimeInState;
    
    // Build weighted list of available actions
    TArray<TPair<FString, float>> AvailableActions;
    float TotalWeight = 0.0f;
    
    for (auto& Pair : CombatActions)
    {
        const FCombatAction& Action = Pair.Value;
        
        // Check if action is on cooldown
        if (CurrentTime - Action.LastUsedTime < Action.Cooldown) continue;
        
        // Check if in range
        if (Distance < Action.MinRange || Distance > Action.MaxRange) continue;
        
        // Check ability-specific cooldowns
        if (IsAbilityOnCooldown(Enemy, Action.ActionName)) continue;
        
        AvailableActions.Add(TPair<FString, float>(Pair.Key, Action.Weight));
        TotalWeight += Action.Weight;
    }
    
    if (AvailableActions.Num() == 0) return TEXT("");
    
    // Weighted random selection
    float RandomValue = FMath::RandRange(0.0f, TotalWeight);
    float CurrentWeight = 0.0f;
    
    for (const auto& ActionPair : AvailableActions)
    {
        CurrentWeight += ActionPair.Value;
        if (RandomValue <= CurrentWeight)
        {
            return ActionPair.Key;
        }
    }
    
    return AvailableActions[0].Key; // Fallback
}

void UCombatState::AddCombatAction(const FString& Name, float Weight, float Cooldown, float MinRange, float MaxRange)
{
    FCombatAction NewAction;
    NewAction.ActionName = Name;
    NewAction.Weight = Weight;
    NewAction.Cooldown = Cooldown;
    NewAction.MinRange = MinRange;
    NewAction.MaxRange = MaxRange;
    
    CombatActions.Add(Name, NewAction);
}

void UCombatState::UpdateCombatPosition(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    AAIController* AIController = Cast<AAIController>(Enemy->GetController());
    if (!AIController || !StateMachine->GetTarget()) return;
    
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    float Distance = GetDistanceToPlayer(Enemy);
    FVector ToPlayer = (StateMachine->GetTarget()->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
    
    FVector DesiredLocation;
    
    // Use enemy's minimum engagement distance
    float MinDistance = Enemy->MinimumEngagementDistance;
    
    // More aggressive positioning - only back up if very close
    if (Distance < MinDistance * 0.8f)  // Too close
    {
        // Back up to minimum distance
        DesiredLocation = Enemy->GetActorLocation() - ToPlayer * (MinDistance - Distance + 50.0f);
    }
    else if (Distance > Params.AttackRange * 0.8f)  // Close in if outside attack range
    {
        // Too far - move to optimal attack position
        DesiredLocation = StateMachine->GetTarget()->GetActorLocation() - ToPlayer * MinDistance;
        
        // Move directly to target with higher acceptance radius for faster approach
        AIController->MoveToLocation(DesiredLocation, 30.0f);
        return;
    }
    else
    {
        // Good distance - aggressive strafing to find openings
        FVector RightVector = FVector::CrossProduct(ToPlayer, FVector::UpVector);
        float StrafeDirection = (FMath::Sin(TimeInState * 2.0f) > 0) ? 1.0f : -1.0f; // Predictable strafe pattern
        DesiredLocation = Enemy->GetActorLocation() + RightVector * StrafeDirection * 100.0f;
    }
    
    AIController->MoveToLocation(DesiredLocation, 30.0f);  // Smaller acceptance radius for tighter movement
}

bool UCombatState::ShouldDefend(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const
{
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    
    // Reactive defense chance is higher
    return FMath::RandRange(0.0f, 1.0f) < Params.ReactiveDefenseChance;
}

void UCombatState::OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (!Enemy || !StateMachine) return;
    
    // React to player dash - potentially dodge or block
    if (ShouldDefend(Enemy, StateMachine))
    {
        // Execute defensive action immediately
        if (Enemy->CanDodge() && !IsAbilityOnCooldown(Enemy, TEXT("Dodge")))
        {
            ExecuteCombatAction(Enemy, StateMachine, TEXT("Dodge"));
        }
    }
}

void UCombatState::OnPlayerAttacking(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (!Enemy || !StateMachine) return;
    
    // React to player attack - block if possible
    if (ShouldDefend(Enemy, StateMachine))
    {
        if (Enemy->CanBlock() && !IsAbilityOnCooldown(Enemy, TEXT("Block")))
        {
            ExecuteCombatAction(Enemy, StateMachine, TEXT("Block"));
        }
    }
}

void UCombatState::OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage)
{
    // After taking damage, might change behavior
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    
    if (GetHealthPercent(Enemy) <= Params.DefensiveHealthPercent)
    {
        StateMachine->ChangeState(EEnemyState::Defensive);
    }
}