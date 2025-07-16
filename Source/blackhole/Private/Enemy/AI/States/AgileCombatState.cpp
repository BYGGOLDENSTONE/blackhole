#include "Enemy/AI/States/AgileCombatState.h"
#include "Enemy/AgileEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/DodgeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "Engine/Engine.h"
#include "Navigation/PathFollowingComponent.h"

void UAgileCombatState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    // Start in approaching phase
    CurrentPhase = EAgileCombatPhase::Approaching;
    bHasExecutedBackstab = false;
    TimeInCurrentPhase = 0.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("Agile Enemy: Entering combat - starting assassin approach"));
}

void UAgileCombatState::Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Exit(Enemy, StateMachine);
    
    // Clear any pending timers
    if (Enemy && Enemy->GetWorld())
    {
        Enemy->GetWorld()->GetTimerManager().ClearTimer(DashAttackTimerHandle);
    }
    
    // Reset assassin state
    CurrentPhase = EAgileCombatPhase::Approaching;
    bHasExecutedBackstab = false;
    bIsCircleStrafing = false;
}

void UAgileCombatState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    // Call parent to update base state timers
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!StateMachine || !StateMachine->GetTarget()) return;
    
    // Update phase timer
    TimeInCurrentPhase += DeltaTime;
    
    // Update assassin behavior instead of default combat
    UpdateAssassinBehavior(Enemy, StateMachine, DeltaTime);
}

void UAgileCombatState::InitializeCombatActions(ABaseEnemy* Enemy)
{
    // Add a dummy action to prevent base class from transitioning to Chase
    // This action will never be executed (we handle everything in UpdateAssassinBehavior)
    AddCombatAction(TEXT("AssassinPattern"), 1.0f, 0.1f, 0.0f, 10000.0f);
}

void UAgileCombatState::ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName)
{
    // Do nothing - all combat is handled in UpdateAssassinBehavior
    // This prevents the base class from executing actions
}

void UAgileCombatState::ExecuteQuickStrike(ABaseEnemy* Enemy)
{
    if (USmashAbilityComponent* SmashAbility = Enemy->FindComponentByClass<USmashAbilityComponent>())
    {
        // Quick strike - less damage but faster
        float OriginalDamage = SmashAbility->GetDamage();
        SmashAbility->SetDamage(OriginalDamage * 0.7f);
        SmashAbility->Execute();
        SmashAbility->SetDamage(OriginalDamage);
        
        // Minimal recovery time
        Enemy->ApplyMovementSpeedModifier(0.8f, 0.3f);
    }
}

void UAgileCombatState::ExecuteDodgeManeuver(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (UDodgeComponent* DodgeAbility = Enemy->FindComponentByClass<UDodgeComponent>())
    {
        DodgeAbility->Execute();
        
        // After dodge, position for counter attack
        if (StateMachine && StateMachine->GetTarget())
        {
            FVector ToTarget = (StateMachine->GetTarget()->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
            FVector DodgeDirection = FVector::CrossProduct(ToTarget, FVector::UpVector);
            
            // Randomly dodge left or right
            if (FMath::RandBool())
            {
                DodgeDirection = -DodgeDirection;
            }
            
            // Apply dodge movement
            if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
            {
                Movement->AddImpulse(DodgeDirection * 1000.0f, true);
            }
        }
    }
}

void UAgileCombatState::ExecuteDashAttack(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    if (!StateMachine || !StateMachine->GetTarget()) return;
    
    AActor* Target = StateMachine->GetTarget();
    FVector TargetLocation = Target->GetActorLocation();
    FVector EnemyLocation = Enemy->GetActorLocation();
    
    // Get target's velocity for prediction
    FVector TargetVelocity = FVector::ZeroVector;
    if (APawn* TargetPawn = Cast<APawn>(Target))
    {
        if (UMovementComponent* MovComp = TargetPawn->GetMovementComponent())
        {
            TargetVelocity = MovComp->Velocity;
        }
    }
    
    // Predict where target will be in 0.3 seconds (dash duration)
    FVector PredictedTargetLocation = TargetLocation + (TargetVelocity * 0.3f);
    
    // Calculate position behind predicted target location
    FVector TargetForward = Target->GetActorForwardVector();
    
    // Use configurable dash behind distance from agile enemy
    AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy);
    float BehindDistance = Agile ? Agile->DashBehindDistance : 200.0f;
    FVector DashTarget = PredictedTargetLocation - (TargetForward * BehindDistance);
    
    // Ensure dash target is at same height as enemy (no vertical dash)
    DashTarget.Z = EnemyLocation.Z;
    
    FVector DashDirection = (DashTarget - EnemyLocation).GetSafeNormal();
    
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        // Stop current movement first to ensure clean dash
        Movement->StopMovementImmediately();
        
        // Fast dash towards behind position
        Movement->AddImpulse(DashDirection * 3000.0f, true);
        
        // Schedule attack at end of dash
        if (Enemy->GetWorld())
        {
            // Clear any existing timer
            Enemy->GetWorld()->GetTimerManager().ClearTimer(DashAttackTimerHandle);
            
            // Use weak pointer to prevent crashes if enemy is destroyed
            TWeakObjectPtr<ABaseEnemy> WeakEnemy = Enemy;
            TWeakObjectPtr<AActor> WeakTarget = Target;
            
            Enemy->GetWorld()->GetTimerManager().SetTimer(DashAttackTimerHandle, [WeakEnemy, WeakTarget]()
            {
                if (WeakEnemy.IsValid() && WeakTarget.IsValid())
                {
                    // Turn to face target before attack
                    FVector LookDirection = (WeakTarget->GetActorLocation() - WeakEnemy->GetActorLocation()).GetSafeNormal();
                    FRotator NewRotation = LookDirection.Rotation();
                    WeakEnemy->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
                    
                    if (USmashAbilityComponent* SmashAbility = WeakEnemy->FindComponentByClass<USmashAbilityComponent>())
                    {
                        // Backstab does increased damage
                        float OriginalDamage = SmashAbility->GetDamage();
                        SmashAbility->SetDamage(OriginalDamage * 2.0f); // Double damage for backstab
                        SmashAbility->Execute();
                        SmashAbility->SetDamage(OriginalDamage); // Reset damage
                        
                        UE_LOG(LogTemp, Warning, TEXT("Agile DashAttack: Executed backstab! Base: %.0f -> Backstab: %.0f damage"), OriginalDamage, OriginalDamage * 2.0f);
                    }
                }
            }, 0.3f, false);
        }
    }
}

void UAgileCombatState::OnPlayerDashed(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    // Agile enemies are very reactive to player dashes
    if (!Enemy || !StateMachine) return;
    
    // Higher chance to dodge when player dashes
    if (FMath::RandRange(0.0f, 1.0f) < 0.7f && !IsAbilityOnCooldown(Enemy, TEXT("Dodge")))
    {
        ExecuteDodgeManeuver(Enemy, StateMachine);
        StartAbilityCooldown(Enemy, TEXT("Dodge"), 1.0f);
    }
}

void UAgileCombatState::UpdateAssassinBehavior(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    if (!Enemy || !StateMachine || !StateMachine->GetTarget()) return;
    
    AActor* Target = StateMachine->GetTarget();
    float DistanceToTarget = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
    bool bDashOnCooldown = IsAbilityOnCooldown(Enemy, TEXT("DashAttack"));
    
    // Update phase based on current state
    switch (CurrentPhase)
    {
        case EAgileCombatPhase::Approaching:
        {
            // Chase the player until within dash range
            if (DistanceToTarget <= DashEngageRange && !bDashOnCooldown)
            {
                // In range and dash is ready - execute dash attack
                CurrentPhase = EAgileCombatPhase::DashAttack;
                TimeInCurrentPhase = 0.0f;
                ExecuteDashAttack(Enemy, StateMachine);
                AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy);
                float DashCD = Agile ? Agile->DashCooldown : 3.0f;
                StartAbilityCooldown(Enemy, TEXT("DashAttack"), DashCD);
                
                UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: Initiating dash backstab attack!"));
            }
            else
            {
                // Move toward player
                if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                {
                    AIController->MoveToActor(Target, 50.0f);
                }
            }
            break;
        }
        
        case EAgileCombatPhase::DashAttack:
        {
            // Wait for dash attack to complete
            if (TimeInCurrentPhase > 0.5f) // Give time for dash + backstab
            {
                CurrentPhase = EAgileCombatPhase::Retreating;
                TimeInCurrentPhase = 0.0f;
                bHasExecutedBackstab = true;
                
                UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: Backstab complete, retreating!"));
            }
            break;
        }
        
        case EAgileCombatPhase::Retreating:
        {
            // Retreat to safe distance
            if (DistanceToTarget >= RetreatDistance)
            {
                CurrentPhase = EAgileCombatPhase::Maintaining;
                TimeInCurrentPhase = 0.0f;
                
                // Reset to normal speed
                if (AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy))
                {
                    Enemy->GetCharacterMovement()->MaxWalkSpeed = Agile->MovementSpeed;
                }
                
                UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: Reached safe distance, maintaining position"));
            }
            else
            {
                // Move away from player with urgency
                FVector RetreatPos = GetRetreatPosition(Enemy, StateMachine);
                if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                {
                    // Stop current movement first
                    AIController->StopMovement();
                    
                    // Move to retreat position with higher priority
                    FAIMoveRequest MoveRequest(RetreatPos);
                    MoveRequest.SetAcceptanceRadius(30.0f);
                    MoveRequest.SetUsePathfinding(true);
                    AIController->MoveTo(MoveRequest);
                    
                    // Apply speed boost during retreat
                    if (AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy))
                    {
                        Enemy->GetCharacterMovement()->MaxWalkSpeed = Agile->MovementSpeed * 1.2f; // 20% faster retreat
                    }
                }
                
                // Face the player while retreating
                FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
                FRotator LookAtRotation = ToTarget.Rotation();
                Enemy->SetActorRotation(FRotator(0.0f, LookAtRotation.Yaw, 0.0f));
            }
            break;
        }
        
        case EAgileCombatPhase::Maintaining:
        {
            // Keep distance until dash is ready
            if (!bDashOnCooldown)
            {
                // Dash is ready - go back to approaching
                CurrentPhase = EAgileCombatPhase::Approaching;
                TimeInCurrentPhase = 0.0f;
                bHasExecutedBackstab = false;
                
                UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: Dash ready, beginning new approach"));
            }
            else
            {
                // Maintain distance between 650-750 units
                if (DistanceToTarget < MaintainDistanceMin)
                {
                    // Too close - back up
                    FVector RetreatPos = GetRetreatPosition(Enemy, StateMachine);
                    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                    {
                        AIController->MoveToLocation(RetreatPos, 50.0f);
                    }
                }
                else if (DistanceToTarget > MaintainDistanceMax)
                {
                    // Too far - move closer slightly
                    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                    {
                        AIController->MoveToActor(Target, MaintainDistanceMin);
                    }
                }
                else
                {
                    // Good distance - strafe to avoid attacks
                    UpdateCircleStrafe(Enemy, StateMachine, DeltaTime);
                }
                
                // Dodge if player attacks
                if (FMath::RandRange(0.0f, 1.0f) < 0.4f && !IsAbilityOnCooldown(Enemy, TEXT("Dodge")))
                {
                    // Check if player is aiming at us
                    FVector PlayerForward = Target->GetActorForwardVector();
                    FVector ToEnemy = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
                    float DotProduct = FVector::DotProduct(PlayerForward, ToEnemy);
                    
                    if (DotProduct > 0.7f) // Player looking at us
                    {
                        ExecuteDodgeManeuver(Enemy, StateMachine);
                        StartAbilityCooldown(Enemy, TEXT("Dodge"), 1.0f);
                    }
                }
            }
            break;
        }
    }
    
    // Show debug info
    if (GEngine)
    {
        FString PhaseString;
        switch (CurrentPhase)
        {
            case EAgileCombatPhase::Approaching: PhaseString = "Approaching"; break;
            case EAgileCombatPhase::DashAttack: PhaseString = "DashAttack"; break;
            case EAgileCombatPhase::Retreating: PhaseString = "Retreating"; break;
            case EAgileCombatPhase::Maintaining: PhaseString = "Maintaining"; break;
        }
        
        float DashCooldownRemaining = StateMachine->GetCooldownRemaining(TEXT("DashAttack"));
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, 
            FString::Printf(TEXT("Agile: %s | Dist: %.0f | Dash CD: %.1fs"), 
                *PhaseString, DistanceToTarget, DashCooldownRemaining));
    }
}

FVector UAgileCombatState::GetRetreatPosition(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const
{
    if (!Enemy || !StateMachine || !StateMachine->GetTarget()) 
        return Enemy->GetActorLocation();
    
    AActor* Target = StateMachine->GetTarget();
    FVector DirectionAway = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
    FVector RetreatPos = Enemy->GetActorLocation() + (DirectionAway * 200.0f); // Move 200 units away
    
    // Make sure the retreat position is navigable
    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Enemy->GetWorld()))
    {
        FNavLocation NavLocation;
        if (NavSys->GetRandomReachablePointInRadius(RetreatPos, 100.0f, NavLocation))
        {
            return NavLocation.Location;
        }
    }
    
    return RetreatPos;
}

bool UAgileCombatState::ShouldRetreat(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) const
{
    // Always retreat after backstab in assassin pattern
    return bHasExecutedBackstab;
}

void UAgileCombatState::UpdateCircleStrafe(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    if (!Enemy || !StateMachine || !StateMachine->GetTarget())
        return;
        
    AActor* Target = StateMachine->GetTarget();
    FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
    
    // Calculate strafe direction (perpendicular to target direction)
    FVector StrafeDirection = FVector::CrossProduct(ToTarget, FVector::UpVector) * CircleStrafeDirection;
    
    // Randomly change direction every 2-4 seconds
    TimeSinceDirectionChange += DeltaTime;
    if (TimeSinceDirectionChange > FMath::RandRange(2.0f, 4.0f))
    {
        CircleStrafeDirection *= -1.0f;
        TimeSinceDirectionChange = 0.0f;
        
        // Sometimes dodge when changing direction
        if (FMath::RandRange(0.0f, 1.0f) < 0.3f && !IsAbilityOnCooldown(Enemy, TEXT("Dodge")))
        {
            ExecuteDodgeManeuver(Enemy, StateMachine);
        }
    }
    
    // Calculate strafe position
    FVector CurrentLoc = Enemy->GetActorLocation();
    FVector StrafeTarget = CurrentLoc + (StrafeDirection * 200.0f);
    
    // Use navigation to move
    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
    {
        // Move to strafe position while facing the target
        AIController->MoveToLocation(StrafeTarget, 10.0f, false, true, false);
        
        // Make enemy face the target while strafing
        FRotator LookAtRotation = ToTarget.Rotation();
        LookAtRotation.Pitch = 0.0f;
        LookAtRotation.Roll = 0.0f;
        Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Agile Enemy: Circle strafing %s"), CircleStrafeDirection > 0 ? TEXT("Right") : TEXT("Left"));
}