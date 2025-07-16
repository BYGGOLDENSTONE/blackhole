#include "Enemy/AI/EnemyStateMachine.h"
#include "Enemy/AI/EnemyStateBase.h"
#include "Enemy/AI/EnemyStates.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

UEnemyStateMachine::UEnemyStateMachine()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // Tick 10 times per second for performance
    
    // Start with tick disabled until properly initialized
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UEnemyStateMachine::BeginPlay()
{
    Super::BeginPlay();
    
    OwnerEnemy = Cast<ABaseEnemy>(GetOwner());
    if (!OwnerEnemy)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyStateMachine: No valid BaseEnemy owner!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("%s StateMachine: BeginPlay - Owner set"), *OwnerEnemy->GetName());
    
    // Don't initialize here - let derived classes do it after setting parameters
    // InitializeStates();
    
    // Start line of sight checking
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(LineOfSightTimer, this, &UEnemyStateMachine::CheckLineOfSight, 0.2f, true);
        UE_LOG(LogTemp, Warning, TEXT("%s StateMachine: Line of sight timer started"), *OwnerEnemy->GetName());
    }
    
    // Don't enter initial state here - do it after states are created
    // EnterState(CurrentState);
}

void UEnemyStateMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear timers first
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(LineOfSightTimer);
        GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    }
    
    // Clean up current state
    if (CurrentStateObject)
    {
        CurrentStateObject->Exit(OwnerEnemy, this);
        CurrentStateObject = nullptr;
    }
    
    // Clean up all state objects
    for (auto& StatePair : States)
    {
        if (StatePair.Value)
        {
            StatePair.Value->RemoveFromRoot();
            // Let garbage collector handle the cleanup
        }
    }
    States.Empty();
    
    // Clear references
    Target = nullptr;
    OwnerEnemy = nullptr;
    
    Super::EndPlay(EndPlayReason);
}

void UEnemyStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Safety check - don't tick if not initialized
    if (!bIsInitialized)
    {
        // Tick called before initialization - skipping
        return;
    }
    
    // Tick logging removed - was too verbose
    
    if (!OwnerEnemy || !CurrentStateObject) 
    {
        if (!OwnerEnemy)
        {
            UE_LOG(LogTemp, Error, TEXT("StateMachine: No owner enemy in tick!"));
        }
        if (!CurrentStateObject)
        {
            // If we're in Dead state, this is expected - just return quietly
            if (CurrentState == EEnemyState::Dead)
            {
                return;
            }
            
            UE_LOG(LogTemp, Error, TEXT("StateMachine: No current state object in tick! Initialized: %s"),
                bIsInitialized ? TEXT("Yes") : TEXT("No"));
            
            // Try to recover by re-entering current state if we have states
            if (States.Contains(CurrentState))
            {
                UE_LOG(LogTemp, Warning, TEXT("StateMachine: Attempting to recover by re-entering state %s"),
                    *UEnum::GetValueAsString(CurrentState));
                EnterState(CurrentState);
                
                // If still no state object, something is very wrong
                if (!CurrentStateObject)
                {
                    UE_LOG(LogTemp, Error, TEXT("StateMachine: Recovery failed - disabling tick"));
                    SetComponentTickEnabled(false);
                }
            }
        }
        return;
    }
    
    // Auto-acquire target if we don't have one
    if (!Target && GetWorld())
    {
        if (AActor* PlayerActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
        {
            SetTarget(PlayerActor);
            UE_LOG(LogTemp, Warning, TEXT("%s: Auto-acquired player target in TickComponent"), *GetName());
        }
    }
    
    TimeInCurrentState += DeltaTime;
    
    // Update cooldowns
    UpdateCooldowns(DeltaTime);
    
    // Update current state
    CurrentStateObject->Update(OwnerEnemy, this, DeltaTime);
    
    // Update last known target location if we have line of sight
    if (Target && bHasLineOfSight)
    {
        UpdateLastKnownTargetLocation();
    }
}

void UEnemyStateMachine::InitializeStates()
{
    CreateDefaultStates();
}

void UEnemyStateMachine::Initialize()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: Initializing state machine"), *GetName());
    
    // This should be called by derived classes after they've set up their parameters
    InitializeStates();
    
    UE_LOG(LogTemp, Warning, TEXT("%s: %d states registered"), *GetName(), States.Num());
    
    // Log target status
    if (Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Target is set: %s"), *GetName(), *Target->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: No target set during initialization!"), *GetName());
    }
    
    // Now enter the initial state
    if (States.Contains(CurrentState))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Entering initial state %s"), 
            *GetName(), 
            *UEnum::GetValueAsString(CurrentState));
        EnterState(CurrentState);
        
        // Verify we have a valid state object
        if (CurrentStateObject)
        {
            bIsInitialized = true;
            SetComponentTickEnabled(true); // Enable tick only after successful initialization
            UE_LOG(LogTemp, Warning, TEXT("%s: State machine initialization complete - tick enabled"), *GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Failed to set current state object!"), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: No state registered for initial state %s! States registered: %d"),
            *GetName(),
            *UEnum::GetValueAsString(CurrentState),
            States.Num());
            
        // Log all registered states
        for (const auto& StatePair : States)
        {
            UE_LOG(LogTemp, Error, TEXT("  - %s"), *UEnum::GetValueAsString(StatePair.Key));
        }
    }
}

void UEnemyStateMachine::CreateDefaultStates()
{
    // This will be overridden by specific enemy types
    // Base implementation creates empty states
}

void UEnemyStateMachine::RegisterState(EEnemyState StateType, UEnemyStateBase* StateObject)
{
    if (!StateObject) 
    {
        UE_LOG(LogTemp, Error, TEXT("RegisterState: Null state object for %s"), *UEnum::GetValueAsString(StateType));
        return;
    }
    
    // Check if state object is valid
    if (!IsValid(StateObject))
    {
        UE_LOG(LogTemp, Error, TEXT("RegisterState: Invalid state object for %s"), *UEnum::GetValueAsString(StateType));
        return;
    }
    
    StateObject->AddToRoot(); // Prevent garbage collection
    States.Add(StateType, StateObject);
    
    UE_LOG(LogTemp, Warning, TEXT("%s: Registered state %s (Object: %s)"), 
        *GetName(), 
        *UEnum::GetValueAsString(StateType),
        *StateObject->GetName());
}

void UEnemyStateMachine::ChangeState(EEnemyState NewState)
{
    if (CurrentState == NewState) return;
    
    // Safety check during shutdown
    if (!OwnerEnemy || !IsValid(OwnerEnemy))
    {
        return;
    }
    
    if (!CanTransitionTo(NewState))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Cannot transition from %s to %s"),
            *OwnerEnemy->GetName(),
            *UEnum::GetValueAsString(CurrentState),
            *UEnum::GetValueAsString(NewState));
        return;
    }
    
    ExitState(CurrentState);
    PreviousState = CurrentState;
    CurrentState = NewState;
    TimeInCurrentState = 0.0f;
    
    // Special handling for Dead state - disable tick to prevent error spam
    if (NewState == EEnemyState::Dead)
    {
        SetComponentTickEnabled(false);
        CurrentStateObject = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("%s: Entered Dead state - tick disabled"), *OwnerEnemy->GetName());
    }
    else
    {
        EnterState(NewState);
    }
    
    OnStateChanged.Broadcast(PreviousState, CurrentState);
}

void UEnemyStateMachine::ForceState(EEnemyState NewState)
{
    ExitState(CurrentState);
    PreviousState = CurrentState;
    CurrentState = NewState;
    TimeInCurrentState = 0.0f;
    EnterState(NewState);
    
    OnStateChanged.Broadcast(PreviousState, CurrentState);
}

void UEnemyStateMachine::EnterState(EEnemyState NewState)
{
    UEnemyStateBase** StatePtr = States.Find(NewState);
    if (StatePtr && *StatePtr)
    {
        CurrentStateObject = *StatePtr;
        CurrentStateObject->Enter(OwnerEnemy, this);
        
        UE_LOG(LogTemp, Verbose, TEXT("%s: Entered state %s - CurrentStateObject set to %s"),
            OwnerEnemy ? *OwnerEnemy->GetName() : TEXT("NoOwner"),
            *UEnum::GetValueAsString(NewState),
            *CurrentStateObject->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: No state object for %s! Available states: %d"),
            OwnerEnemy ? *OwnerEnemy->GetName() : TEXT("NoOwner"),
            *UEnum::GetValueAsString(NewState),
            States.Num());
            
        // Clear current state object to prevent crashes
        CurrentStateObject = nullptr;
    }
}

void UEnemyStateMachine::ExitState(EEnemyState OldState)
{
    if (CurrentStateObject)
    {
        CurrentStateObject->Exit(OwnerEnemy, this);
        // Don't clear CurrentStateObject here - it should be set by EnterState
    }
}

bool UEnemyStateMachine::CanTransitionTo(EEnemyState NewState) const
{
    if (!CurrentStateObject) return true;
    return CurrentStateObject->CanTransitionTo(NewState);
}

void UEnemyStateMachine::SetTarget(AActor* NewTarget)
{
    Target = NewTarget;
    if (Target)
    {
        UpdateLastKnownTargetLocation();
        UE_LOG(LogTemp, Warning, TEXT("%s StateMachine: Target set to %s"), 
            OwnerEnemy ? *OwnerEnemy->GetName() : TEXT("NoOwner"), 
            *Target->GetName());
            
        // If we're already initialized but had no target, we might be stuck in Idle
        // Force a state update
        if (CurrentStateObject && CurrentState == EEnemyState::Idle)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s StateMachine: Forcing idle state update after target set"), 
                *OwnerEnemy->GetName());
            // No need to change state, just let the next update handle it
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s StateMachine: Target cleared"), 
            OwnerEnemy ? *OwnerEnemy->GetName() : TEXT("NoOwner"));
    }
}

void UEnemyStateMachine::UpdateLastKnownTargetLocation()
{
    if (Target)
    {
        LastKnownTargetLocation = Target->GetActorLocation();
    }
}

void UEnemyStateMachine::CheckLineOfSight()
{
    if (!OwnerEnemy || !Target) 
    {
        bHasLineOfSight = false;
        UE_LOG(LogTemp, VeryVerbose, TEXT("%s: CheckLineOfSight - No owner or target"),
            OwnerEnemy ? *OwnerEnemy->GetName() : TEXT("NoOwner"));
        return;
    }
    
    FHitResult HitResult;
    FVector Start = OwnerEnemy->GetActorLocation() + FVector(0, 0, 50); // Eye height
    FVector TargetLoc = Target->GetActorLocation();
    
    // Check multiple target heights to account for wall running
    TArray<FVector> TargetPositions = {
        TargetLoc,                                      // Normal position
        TargetLoc + FVector(0, 0, 100),               // Higher position for wall running
        TargetLoc + FVector(0, 0, 200),               // Even higher for wall running
        TargetLoc - FVector(0, 0, 50)                 // Lower position for crouching/falling
    };
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerEnemy);
    QueryParams.AddIgnoredActor(Target);
    
    // Check line of sight to multiple heights
    bHasLineOfSight = false;
    FVector SuccessfulTargetPos = TargetLoc; // Default to base target location
    
    for (const FVector& End : TargetPositions)
    {
        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            Start,
            End,
            ECC_Visibility,
            QueryParams
        );
        
        if (!bHit)
        {
            bHasLineOfSight = true;
            SuccessfulTargetPos = End; // Store the position where we found line of sight
            break; // Found line of sight at one of the heights
        }
    }
    
    // Debug visualization
    if (GetWorld())
    {
        FColor LineColor = bHasLineOfSight ? FColor::Green : FColor::Red;
        DrawDebugLine(GetWorld(), Start, SuccessfulTargetPos, LineColor, false, 0.2f, 0, 1.0f);
        
        // Always log line of sight status
        static int LOSCounter = 0;
        if (LOSCounter++ % 5 == 0) // Log every second (5 * 0.2s)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Line of sight to %s: %s (Distance: %.0f)"),
                *OwnerEnemy->GetName(), 
                *Target->GetName(),
                bHasLineOfSight ? TEXT("YES") : TEXT("NO"),
                FVector::Dist(Start, SuccessfulTargetPos));
        }
        
        // Debug text
        // Debug message removed - line of sight
    }
}

void UEnemyStateMachine::NotifyPlayerDashed()
{
    if (CurrentStateObject)
    {
        CurrentStateObject->OnPlayerDashed(OwnerEnemy, this);
    }
}

void UEnemyStateMachine::NotifyPlayerAttacking()
{
    if (CurrentStateObject)
    {
        CurrentStateObject->OnPlayerAttacking(OwnerEnemy, this);
    }
}

void UEnemyStateMachine::NotifyPlayerUltimateUsed()
{
    if (CurrentStateObject)
    {
        CurrentStateObject->OnPlayerUltimateUsed(OwnerEnemy, this);
    }
}

void UEnemyStateMachine::NotifyDamageTaken(float Damage)
{
    if (CurrentStateObject)
    {
        CurrentStateObject->OnDamageTaken(OwnerEnemy, this, Damage);
    }
}

void UEnemyStateMachine::StartCooldown(const FString& CooldownName, float Duration)
{
    ActiveCooldowns.Add(CooldownName, Duration);
}

bool UEnemyStateMachine::IsCooldownActive(const FString& CooldownName) const
{
    const float* Remaining = ActiveCooldowns.Find(CooldownName);
    return Remaining && *Remaining > 0.0f;
}

float UEnemyStateMachine::GetCooldownRemaining(const FString& CooldownName) const
{
    const float* Remaining = ActiveCooldowns.Find(CooldownName);
    return Remaining ? *Remaining : 0.0f;
}

void UEnemyStateMachine::UpdateCooldowns(float DeltaTime)
{
    TArray<FString> ExpiredCooldowns;
    
    for (auto& Pair : ActiveCooldowns)
    {
        Pair.Value -= DeltaTime;
        if (Pair.Value <= 0.0f)
        {
            ExpiredCooldowns.Add(Pair.Key);
        }
    }
    
    for (const FString& Key : ExpiredCooldowns)
    {
        ActiveCooldowns.Remove(Key);
    }
}