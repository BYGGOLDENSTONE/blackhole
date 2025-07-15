#include "Enemy/AI/EnemyStateBase.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UEnemyStateBase::UEnemyStateBase()
{
}

void UEnemyStateBase::Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    TimeInState = 0.0f;
    UE_LOG(LogTemp, Warning, TEXT("%s: Entered %s state"), 
        Enemy ? *Enemy->GetName() : TEXT("Unknown"), 
        *UEnum::GetValueAsString(GetStateType()));
}

void UEnemyStateBase::Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine)
{
    UE_LOG(LogTemp, Warning, TEXT("%s: Exited %s state (Duration: %.2fs)"), 
        Enemy ? *Enemy->GetName() : TEXT("Unknown"), 
        *UEnum::GetValueAsString(GetStateType()),
        TimeInState);
}

void UEnemyStateBase::Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime)
{
    TimeInState += DeltaTime;
}

bool UEnemyStateBase::IsPlayerInRange(ABaseEnemy* Enemy, float Range) const
{
    if (!Enemy) 
    {
        UE_LOG(LogTemp, Error, TEXT("IsPlayerInRange: Enemy is NULL!"));
        return false;
    }
    
    // Get the state machine's target instead of enemy's target
    UEnemyStateMachine* StateMachine = Enemy->FindComponentByClass<UEnemyStateMachine>();
    if (!StateMachine) 
    {
        UE_LOG(LogTemp, Error, TEXT("%s: IsPlayerInRange: No StateMachine component found!"), *Enemy->GetName());
        return false;
    }
    
    AActor* Target = StateMachine->GetTarget();
    if (!Target) 
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("%s: IsPlayerInRange: No target set in StateMachine!"), *Enemy->GetName());
        return false;
    }
    
    float Distance = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
    
    bool bInRange = Distance <= Range;
    
    // Always log range check results
    UE_LOG(LogTemp, Verbose, TEXT("%s: IsPlayerInRange - Distance: %.0f, Range: %.0f, InRange: %s"),
        *Enemy->GetName(), Distance, Range, bInRange ? TEXT("YES") : TEXT("NO"));
    
    // Debug message removed - player range check
    
    return bInRange;
}

bool UEnemyStateBase::HasLineOfSightToPlayer(ABaseEnemy* Enemy) const
{
    if (!Enemy) return false;
    
    UEnemyStateMachine* StateMachine = Enemy->FindComponentByClass<UEnemyStateMachine>();
    return StateMachine ? StateMachine->HasLineOfSight() : false;
}

float UEnemyStateBase::GetDistanceToPlayer(ABaseEnemy* Enemy) const
{
    if (!Enemy) return MAX_FLT;
    
    // Get the state machine's target instead of enemy's target
    UEnemyStateMachine* StateMachine = Enemy->FindComponentByClass<UEnemyStateMachine>();
    if (!StateMachine) return MAX_FLT;
    
    AActor* Target = StateMachine->GetTarget();
    if (!Target) return MAX_FLT;
    
    return FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
}

float UEnemyStateBase::GetHealthPercent(ABaseEnemy* Enemy) const
{
    if (!Enemy) return 0.0f;
    
    // Use WP as health
    float MaxHealth = Enemy->GetMaxWP();
    float CurrentHealth = Enemy->GetCurrentWP();
    
    return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

bool UEnemyStateBase::IsAbilityOnCooldown(ABaseEnemy* Enemy, const FString& AbilityName) const
{
    if (!Enemy) return true;
    
    UEnemyStateMachine* StateMachine = Enemy->FindComponentByClass<UEnemyStateMachine>();
    return StateMachine ? StateMachine->IsCooldownActive(AbilityName) : true;
}

void UEnemyStateBase::StartAbilityCooldown(ABaseEnemy* Enemy, const FString& AbilityName, float Duration) const
{
    if (!Enemy) return;
    
    UEnemyStateMachine* StateMachine = Enemy->FindComponentByClass<UEnemyStateMachine>();
    if (StateMachine)
    {
        StateMachine->StartCooldown(AbilityName, Duration);
    }
}

void UEnemyStateBase::RotateTowardsTarget(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime, float RotationSpeed) const
{
    if (!Enemy || !StateMachine) return;
    
    AActor* Target = StateMachine->GetTarget();
    if (!Target) return;
    
    // Get direction to target
    FVector DirectionToTarget = Target->GetActorLocation() - Enemy->GetActorLocation();
    DirectionToTarget.Z = 0.0f; // Keep rotation on horizontal plane
    DirectionToTarget.Normalize();
    
    // Get desired rotation
    FRotator DesiredRotation = DirectionToTarget.Rotation();
    
    // Smoothly interpolate to desired rotation
    FRotator CurrentRotation = Enemy->GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, RotationSpeed);
    
    // Debug logging
    float YawDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, DesiredRotation.Yaw));
    if (YawDiff > 1.0f) // Only log if there's meaningful rotation needed
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("%s: Rotating - Current: %.1f, Desired: %.1f, New: %.1f, Speed: %.1f"),
            *Enemy->GetName(),
            CurrentRotation.Yaw,
            DesiredRotation.Yaw,
            NewRotation.Yaw,
            RotationSpeed);
    }
    
    Enemy->SetActorRotation(NewRotation);
}