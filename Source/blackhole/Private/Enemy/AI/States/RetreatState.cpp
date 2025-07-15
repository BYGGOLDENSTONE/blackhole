#include "Enemy/AI/States/RetreatState.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

void URetreatState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    if (!Enemy) return;
    
    bRetreatTargetSet = false;
    HealStartTime = TimeInState + HealDelay;
    
    // Increase speed for retreat
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = Enemy->GetDefaultWalkSpeed() * 1.2f;
    }
    
    // Find retreat location
    FindRetreatLocation(Enemy, StateMachine);
    
    // Call for backup if low health
    if (GetHealthPercent(Enemy) <= 0.2f)
    {
        CallForBackup(Enemy, StateMachine);
    }
}

void URetreatState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!Enemy || !StateMachine) return;
    
    const FEnemyAIParameters& Params = StateMachine->GetAIParameters();
    
    // Check if we've recovered enough
    if (GetHealthPercent(Enemy) >= Params.DefensiveHealthPercent)
    {
        // Re-engage if player is still in sight
        if (StateMachine->GetTarget() && HasLineOfSightToPlayer(Enemy))
        {
            float Distance = GetDistanceToPlayer(Enemy);
            if (Distance <= Params.ChaseRange)
            {
                StateMachine->ChangeState(EEnemyState::Chase);
                return;
            }
        }
        
        // Otherwise return to idle
        StateMachine->ChangeState(EEnemyState::Idle);
        return;
    }
    
    // Continue retreating
    if (bRetreatTargetSet)
    {
        float DistanceToRetreat = FVector::Dist(Enemy->GetActorLocation(), RetreatTarget);
        
        // If reached retreat location, start healing
        if (DistanceToRetreat < 100.0f)
        {
            // Stop and heal
            if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
            {
                Movement->StopMovementImmediately();
            }
            
            // Apply healing after delay (increase WP back to max)
            if (TimeInState >= HealStartTime)
            {
                float CurrentHealth = Enemy->GetCurrentWP();
                float MaxHealth = Enemy->GetMaxWP();
                
                if (CurrentHealth < MaxHealth)
                {
                    float HealAmount = HealRate * DeltaTime;
                    Enemy->SetCurrentWP(FMath::Min(CurrentHealth + HealAmount, MaxHealth));
                }
            }
        }
        else
        {
            // Keep moving to retreat location
            if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
            {
                AIController->MoveToLocation(RetreatTarget, 50.0f);
            }
        }
    }
}

void URetreatState::FindRetreatLocation(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (!Enemy || !StateMachine) return;
    
    FVector EnemyLocation = Enemy->GetActorLocation();
    FVector AwayFromPlayer = EnemyLocation;
    
    if (AActor* Target = StateMachine->GetTarget())
    {
        FVector ToPlayer = (Target->GetActorLocation() - EnemyLocation).GetSafeNormal();
        AwayFromPlayer = EnemyLocation - ToPlayer * 1000.0f; // Retreat 1000 units away
    }
    
    // Find navigable point
    FNavLocation NavLocation;
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
    
    if (NavSystem && NavSystem->GetRandomReachablePointInRadius(AwayFromPlayer, 500.0f, NavLocation))
    {
        RetreatTarget = NavLocation.Location;
        bRetreatTargetSet = true;
        
        #if WITH_EDITOR
        DrawDebugSphere(Enemy->GetWorld(), RetreatTarget, 100.0f, 12, FColor::Red, false, 5.0f);
        #endif
    }
    else
    {
        // Fallback - just move directly away
        RetreatTarget = AwayFromPlayer;
        bRetreatTargetSet = true;
    }
    
    // Start moving immediately
    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
    {
        AIController->MoveToLocation(RetreatTarget, 50.0f);
    }
}

void URetreatState::CallForBackup(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (!Enemy || !Enemy->GetWorld()) return;
    
    const float CallRadius = 1500.0f;
    
    // Find nearby allies
    for (TActorIterator<ABaseEnemy> It(Enemy->GetWorld()); It; ++It)
    {
        ABaseEnemy* Ally = *It;
        if (!Ally || Ally == Enemy) continue;
        
        float Distance = FVector::Dist(Enemy->GetActorLocation(), Ally->GetActorLocation());
        if (Distance <= CallRadius)
        {
            // Alert ally to player location
            if (UEnemyStateMachine* AllyStateMachine = Ally->FindComponentByClass<UEnemyStateMachine>())
            {
                if (AllyStateMachine->GetCurrentState() == EEnemyState::Idle || 
                    AllyStateMachine->GetCurrentState() == EEnemyState::Patrol)
                {
                    AllyStateMachine->SetTarget(StateMachine->GetTarget());
                    AllyStateMachine->UpdateLastKnownTargetLocation();
                    AllyStateMachine->ChangeState(EEnemyState::Alert);
                    
                    UE_LOG(LogTemp, Warning, TEXT("%s called for backup from %s"), 
                        *Enemy->GetName(), *Ally->GetName());
                }
            }
        }
    }
}

void URetreatState::OnDamageTaken(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float Damage)
{
    // Taking damage while retreating - find new retreat location
    FindRetreatLocation(Enemy, StateMachine);
    HealStartTime = TimeInState + HealDelay; // Reset heal timer
}