#pragma once

#include "CoreMinimal.h"
#include "EnemyStates.generated.h"

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
    None            UMETA(DisplayName = "None"),
    Idle            UMETA(DisplayName = "Idle"),
    Patrol          UMETA(DisplayName = "Patrol"),
    Alert           UMETA(DisplayName = "Alert/Search"),
    Chase           UMETA(DisplayName = "Chase"),
    Combat          UMETA(DisplayName = "Combat"),
    Defensive       UMETA(DisplayName = "Defensive"),
    Retreat         UMETA(DisplayName = "Retreat"),
    Cooldown        UMETA(DisplayName = "Cooldown"),
    Stunned         UMETA(DisplayName = "Stunned"),
    Channeling      UMETA(DisplayName = "Channeling Ability"),
    Dead            UMETA(DisplayName = "Dead")
};

USTRUCT(BlueprintType)
struct FEnemyStateTransition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Machine")
    EEnemyState FromState = EEnemyState::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Machine")
    EEnemyState ToState = EEnemyState::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Machine")
    float Priority = 0.0f;

    FEnemyStateTransition() {}
    FEnemyStateTransition(EEnemyState From, EEnemyState To, float Prio = 0.0f)
        : FromState(From), ToState(To), Priority(Prio) {}
};

USTRUCT(BlueprintType)
struct FEnemyAIParameters
{
    GENERATED_BODY()

    // Health thresholds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RetreatHealthPercent = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DefensiveHealthPercent = 0.5f;

    // Distance thresholds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    float AttackRange = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    float ChaseRange = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    float SightRange = 4500.0f;  // Default sight range (updated from 2500)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    float PreferredCombatDistance = 200.0f;

    // Timing parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float ReactionTime = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float SearchDuration = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float MaxTimeInCombat = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float PatrolWaitTime = 2.0f;

    // Combat parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DodgeChance = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BlockChance = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ReactiveDefenseChance = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackCooldown = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AbilityCooldown = 5.0f;

    // Aggression level (affects chase persistence, attack frequency)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AggressionLevel = 0.5f;
};