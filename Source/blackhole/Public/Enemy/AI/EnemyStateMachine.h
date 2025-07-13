#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyStates.h"
#include "EnemyStateMachine.generated.h"

class UEnemyStateBase;
class ABaseEnemy;
class ABlackholePlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, EEnemyState, OldState, EEnemyState, NewState);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UEnemyStateMachine : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyStateMachine();

    // Lifecycle
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // State management
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void ChangeState(EEnemyState NewState);

    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void ForceState(EEnemyState NewState);

    UFUNCTION(BlueprintPure, Category = "State Machine")
    EEnemyState GetCurrentState() const { return CurrentState; }

    UFUNCTION(BlueprintPure, Category = "State Machine")
    float GetTimeInCurrentState() const { return TimeInCurrentState; }

    // Initialization - must be called by derived classes after setup
    void Initialize();
    
    // Player reactions
    void NotifyPlayerDashed();
    void NotifyPlayerAttacking();
    void NotifyPlayerUltimateUsed();
    void NotifyDamageTaken(float Damage);

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void SetAIParameters(const FEnemyAIParameters& NewParams) { AIParameters = NewParams; }

    UFUNCTION(BlueprintPure, Category = "State Machine")
    const FEnemyAIParameters& GetAIParameters() const { return AIParameters; }

    // Target management
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void SetTarget(AActor* NewTarget);

    UFUNCTION(BlueprintPure, Category = "State Machine")
    AActor* GetTarget() const { return Target; }

    UFUNCTION(BlueprintPure, Category = "State Machine")
    FVector GetLastKnownTargetLocation() const { return LastKnownTargetLocation; }

    void UpdateLastKnownTargetLocation();

    // Cooldown management
    void StartCooldown(const FString& CooldownName, float Duration);
    bool IsCooldownActive(const FString& CooldownName) const;
    float GetCooldownRemaining(const FString& CooldownName) const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "State Machine")
    FOnStateChanged OnStateChanged;

protected:
    // State management
    UPROPERTY()
    TMap<EEnemyState, UEnemyStateBase*> States;

    UPROPERTY()
    EEnemyState CurrentState = EEnemyState::Idle;

    UPROPERTY()
    EEnemyState PreviousState = EEnemyState::None;

    UPROPERTY()
    UEnemyStateBase* CurrentStateObject = nullptr;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Config")
    FEnemyAIParameters AIParameters;

    // Runtime data
    UPROPERTY()
    AActor* Target = nullptr;

    UPROPERTY()
    ABaseEnemy* OwnerEnemy = nullptr;

    FVector LastKnownTargetLocation;
    float TimeInCurrentState = 0.0f;

    // Cooldowns
    TMap<FString, float> ActiveCooldowns;

    // Initialization
    virtual void InitializeStates();
    virtual void CreateDefaultStates();
    void RegisterState(EEnemyState StateType, UEnemyStateBase* StateObject);

    // State transitions
    bool CanTransitionTo(EEnemyState NewState) const;
    void EnterState(EEnemyState NewState);
    void ExitState(EEnemyState OldState);
    void UpdateCooldowns(float DeltaTime);

private:
    // Line of sight checking
    FTimerHandle LineOfSightTimer;
    void CheckLineOfSight();
    bool bHasLineOfSight = false;
    
    // Initialization tracking
    bool bIsInitialized = false;

public:
    UFUNCTION(BlueprintPure, Category = "State Machine")
    bool HasLineOfSight() const { return bHasLineOfSight; }
};