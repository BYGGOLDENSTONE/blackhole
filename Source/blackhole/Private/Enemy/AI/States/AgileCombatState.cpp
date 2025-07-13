#include "Enemy/AI/States/AgileCombatState.h"
#include "Enemy/AgileEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/DodgeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UAgileCombatState::Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    Super::Exit(Enemy, StateMachine);
    
    // Clear any pending timers
    if (Enemy && Enemy->GetWorld())
    {
        Enemy->GetWorld()->GetTimerManager().ClearTimer(DashAttackTimerHandle);
    }
    
    // Reset circle strafe state
    bIsCircleStrafing = false;
}

void UAgileCombatState::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    Super::Update(Enemy, StateMachine, DeltaTime);
    
    if (!StateMachine || !StateMachine->GetTarget()) return;
    
    float DistanceToTarget = FVector::Dist(Enemy->GetActorLocation(), StateMachine->GetTarget()->GetActorLocation());
    bool bDashOnCooldown = IsAbilityOnCooldown(Enemy, TEXT("DashAttack"));
    
    // Agile enemy behavior based on dash availability
    if (bDashOnCooldown)
    {
        // When dash is on cooldown, maintain distance and circle strafe
        if (DistanceToTarget < 400.0f)
        {
            // Too close - retreat while facing target
            FVector RetreatDirection = (Enemy->GetActorLocation() - StateMachine->GetTarget()->GetActorLocation()).GetSafeNormal();
            FVector RetreatLocation = Enemy->GetActorLocation() + (RetreatDirection * 200.0f);
            
            if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
            {
                AIController->MoveToLocation(RetreatLocation, 10.0f);
                Enemy->SetActorRotation((-RetreatDirection).Rotation());
            }
            bIsCircleStrafing = false;
        }
        else if (DistanceToTarget > 400.0f && DistanceToTarget < 800.0f)
        {
            // Optimal range - circle strafe
            bIsCircleStrafing = true;
            UpdateCircleStrafe(Enemy, StateMachine, DeltaTime);
        }
        else
        {
            // Too far - maintain distance but don't get too close
            bIsCircleStrafing = false;
        }
    }
    else
    {
        // Dash available - stop strafing to prepare for attack
        bIsCircleStrafing = false;
        
        // If we're at good dash range (500-800), combat state will handle the dash attack
        // If too close (<300), let normal combat handle it
    }
}

void UAgileCombatState::InitializeCombatActions(ABaseEnemy* Enemy)
{
    // Agile actions: Quick attacks, dodges, and dash attacks
    AddCombatAction(TEXT("QuickStrike"), 3.0f, 1.5f, 0.0f, 200.0f);    // Fast melee - slightly shorter range
    AddCombatAction(TEXT("Dodge"), 2.5f, 1.0f, 0.0f, 400.0f);          // Evasive maneuver
    AddCombatAction(TEXT("DashAttack"), 2.0f, 3.0f, 150.0f, 800.0f);  // Gap closer - increased range and weight
}

void UAgileCombatState::ExecuteCombatAction(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, const FString& ActionName)
{
    if (!Enemy) return;
    
    AAgileEnemy* Agile = Cast<AAgileEnemy>(Enemy);
    if (!Agile) return;
    
    if (ActionName == TEXT("QuickStrike"))
    {
        ExecuteQuickStrike(Agile);
        StartAbilityCooldown(Enemy, TEXT("QuickStrike"), 1.5f);
        bLastActionWasDodge = false;
    }
    else if (ActionName == TEXT("Dodge"))
    {
        ExecuteDodgeManeuver(Agile, StateMachine);
        StartAbilityCooldown(Enemy, TEXT("Dodge"), 1.0f);
        bLastActionWasDodge = true;
    }
    else if (ActionName == TEXT("DashAttack"))
    {
        ExecuteDashAttack(Agile, StateMachine);
        StartAbilityCooldown(Enemy, TEXT("DashAttack"), 3.0f);
        bLastActionWasDodge = false;
    }
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
    FVector DashTarget = PredictedTargetLocation - (TargetForward * 200.0f); // 200 units behind (increased from 150)
    
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
                        SmashAbility->Execute();
                        UE_LOG(LogTemp, Warning, TEXT("Agile DashAttack: Executed backstab"));
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