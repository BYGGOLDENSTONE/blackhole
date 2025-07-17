#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateBase.h"
#include "StandardBuildingState.generated.h"

class UBuilderComponent;

UCLASS()
class BLACKHOLE_API UStandardBuildingState : public UEnemyStateBase
{
    GENERATED_BODY()

public:
    UStandardBuildingState();

    virtual void Enter(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    virtual void Update(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine, float DeltaTime) override;
    virtual void Exit(ABaseEnemy* Enemy, UEnemyStateMachine* StateMachine) override;
    
    virtual EEnemyState GetStateType() const override { return EEnemyState::Building; }

protected:
    UPROPERTY()
    UBuilderComponent* CachedBuilderComponent;
    
    FVector BuildLocation;
    float TimeOutOfRange;
    
    // Maximum time allowed outside build radius before returning
    UPROPERTY(EditDefaultsOnly, Category = "Building", meta = (DisplayName = "Max Time Out Of Range"))
    float MaxTimeOutOfRange = 2.0f;
    
    // Distance from build location to maintain
    UPROPERTY(EditDefaultsOnly, Category = "Building", meta = (DisplayName = "Stay In Range Distance"))
    float StayInRangeDistance = 400.0f;
    
    void MoveTowardsBuildLocation(ABaseEnemy* Enemy, float DeltaTime);
    bool IsInBuildRange(ABaseEnemy* Enemy) const;
};