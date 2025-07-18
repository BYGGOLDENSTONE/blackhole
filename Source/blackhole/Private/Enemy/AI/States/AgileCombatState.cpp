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
#include "Player/BlackholePlayerCharacter.h"

void UAgileCombatState::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Enter(Enemy, StateMachine);
    
    // Start in approaching phase
    CurrentPhase = EAgileCombatPhase::Approaching;
    bHasExecutedBackstab = false;
    TimeInCurrentPhase = 0.0f;
    
    // UE_LOG(LogTemp, Warning, TEXT("Agile Enemy: Entering combat - starting assassin approach"));
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
    // Add Assassin Approach as the only combat action
    // This prevents base class from transitioning to Chase
    AddCombatAction(TEXT("AssassinApproach"), 1.0f, 0.1f, 0.0f, 10000.0f);
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
    
    // Calculate dash to overshoot past the player
    FVector DirectionToTarget = (PredictedTargetLocation - EnemyLocation).GetSafeNormal();
    
    // Use configurable dash behind distance from agile enemy
    AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy);
    float BehindDistance = Agile ? Agile->DashBehindDistance : 250.0f;
    
    // Overshoot past the player's position - but not too far
    float DistanceToTarget = FVector::Dist(EnemyLocation, PredictedTargetLocation);
    float DashDistance = DistanceToTarget + 150.0f; // Reduced overshoot for better hit detection
    
    FVector DashTarget = EnemyLocation + (DirectionToTarget * DashDistance);
    
    // Ensure dash target is at same height as enemy (no vertical dash)
    DashTarget.Z = EnemyLocation.Z;
    
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        // Stop current movement first to ensure clean dash
        Movement->StopMovementImmediately();
        
        // Calculate dash force based on distance
        float DashForce = FMath::Clamp(DashDistance * 10.0f, 3000.0f, 5000.0f);
        
        // Fast dash through and past the player
        Movement->AddImpulse(DirectionToTarget * DashForce, true);
        
        // UE_LOG(LogTemp, Warning, TEXT("Assassin Approach: Dashing %.0f units to get behind player"), DashDistance);
        
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
                        // Get agile enemy for configuration
                        AAgileEnemy* AgileEnemy = Cast<AAgileEnemy>(WeakEnemy.Get());
                        
                        // Temporarily use area damage for backstab to ensure hit
                        bool bOriginalAreaDamage = SmashAbility->bIsAreaDamage;
                        float OriginalRadius = SmashAbility->GetAreaRadius();
                        SmashAbility->SetAreaDamage(true);
                        SmashAbility->SetAreaRadius(200.0f); // Small radius to hit player behind us
                        
                        // Backstab does increased damage
                        float OriginalDamage = SmashAbility->GetDamage();
                        float DamageMultiplier = AgileEnemy ? AgileEnemy->BackstabDamageMultiplier : 2.0f;
                        SmashAbility->SetDamage(OriginalDamage * DamageMultiplier); // Configurable backstab damage
                        SmashAbility->Execute();
                        
                        // Reset all values
                        SmashAbility->SetDamage(OriginalDamage);
                        SmashAbility->SetAreaDamage(bOriginalAreaDamage);
                        SmashAbility->SetAreaRadius(OriginalRadius);
                        
                        // Apply stagger to player on backstab hit
                        if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(WeakTarget.Get()))
                        {
                            float StaggerDuration = AgileEnemy ? AgileEnemy->BackstabStaggerDuration : 1.5f;
                            Player->ApplyStagger(StaggerDuration);
                            // UE_LOG(LogTemp, Warning, TEXT("Agile Backstab: Applied %.1fs stagger to player"), StaggerDuration);
                        }
                        
                        // UE_LOG(LogTemp, Warning, TEXT("Agile DashAttack: Executed backstab! Base: %.0f -> Backstab: %.0f damage (x%.1f)"), OriginalDamage, OriginalDamage * DamageMultiplier, DamageMultiplier);
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
            // If dash is ready and we're in range - ATTACK!
            if (!bDashOnCooldown && DistanceToTarget <= 600.0f) // Increased dash range
            {
                // Dash is ready - execute assassin approach
                CurrentPhase = EAgileCombatPhase::DashAttack;
                TimeInCurrentPhase = 0.0f;
                ExecuteDashAttack(Enemy, StateMachine);
                AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy);
                float DashCD = Agile ? Agile->DashCooldown : 3.0f;
                StartAbilityCooldown(Enemy, TEXT("DashAttack"), DashCD);
                
                // UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: Executing Assassin Approach from %.0f units!"), DistanceToTarget);
            }
            else if (!bDashOnCooldown && DistanceToTarget > 600.0f)
            {
                // Dash ready but too far - move closer aggressively
                if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                {
                    AIController->MoveToActor(Target, 500.0f);
                }
            }
            else
            {
                // Dash on cooldown - maintain distance
                if (DistanceToTarget < 400.0f)
                {
                    // Too close - back away
                    FVector RetreatPos = GetRetreatPosition(Enemy, StateMachine);
                    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                    {
                        AIController->MoveToLocation(RetreatPos, 50.0f);
                    }
                }
                else if (DistanceToTarget > 600.0f)
                {
                    // Too far - move to optimal distance
                    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                    {
                        AIController->MoveToActor(Target, 500.0f);
                    }
                }
                else
                {
                    // Good distance - circle strafe
                    UpdateCircleStrafe(Enemy, StateMachine, DeltaTime);
                }
                
                // Always face the target
                FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
                FRotator LookAtRotation = ToTarget.Rotation();
                Enemy->SetActorRotation(FRotator(0.0f, LookAtRotation.Yaw, 0.0f));
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
                
                // UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: Backstab complete, retreating!"));
            }
            break;
        }
        
        case EAgileCombatPhase::Retreating:
        {
            // Retreat for configurable duration
            AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy);
            float RetreatTime = Agile ? Agile->RetreatDuration : 3.0f;
            
            if (TimeInCurrentPhase >= RetreatTime)
            {
                CurrentPhase = EAgileCombatPhase::Maintaining;
                TimeInCurrentPhase = 0.0f;
                TimeInMaintainPhase = 0.0f; // Reset maintain timer
                
                // Reset to normal speed
                if (Agile)
                {
                    Enemy->GetCharacterMovement()->MaxWalkSpeed = Agile->MovementSpeed;
                }
                
                // UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: %.1f second retreat complete, maintaining position"), RetreatTime);
            }
            else
            {
                // Always move away from player during retreat
                FVector DirectionAway = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
                FVector RetreatPos = Enemy->GetActorLocation() + (DirectionAway * 300.0f);
                
                if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                {
                    AIController->MoveToLocation(RetreatPos, 30.0f);
                    
                    // Apply speed boost during retreat
                    if (Agile)
                    {
                        Enemy->GetCharacterMovement()->MaxWalkSpeed = Agile->MovementSpeed * 1.5f; // 50% faster retreat
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
            // Track time in maintain phase
            TimeInMaintainPhase += DeltaTime;
            
            // Force attack after 5 seconds OR when dash is ready
            bool bForceAttack = TimeInMaintainPhase >= MaxMaintainTime;
            
            if (!bDashOnCooldown || bForceAttack)
            {
                // Dash is ready or we've waited too long - attack now!
                CurrentPhase = EAgileCombatPhase::Approaching;
                TimeInCurrentPhase = 0.0f;
                TimeInMaintainPhase = 0.0f;
                bHasExecutedBackstab = false;
                
                if (bForceAttack)
                {
                    // UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: Forcing attack after %.1f seconds!"), MaxMaintainTime);
                }
                else
                {
                    // UE_LOG(LogTemp, Warning, TEXT("Agile Assassin: Dash ready, beginning new approach"));
                }
            }
            else
            {
                // Maintain distance between configurable min/max units
                AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy);
                float MinDist = Agile ? Agile->MaintainDistanceMin : 650.0f;
                float MaxDist = Agile ? Agile->MaintainDistanceMax : 750.0f;
                
                if (DistanceToTarget < MinDist)
                {
                    // Too close - back up
                    FVector RetreatPos = GetRetreatPosition(Enemy, StateMachine);
                    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                    {
                        AIController->MoveToLocation(RetreatPos, 50.0f);
                    }
                }
                else if (DistanceToTarget > MaxDist)
                {
                    // Too far - move closer slightly
                    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
                    {
                        AIController->MoveToActor(Target, MinDist);
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