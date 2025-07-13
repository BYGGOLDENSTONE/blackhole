#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "TankEnemyStateMachine.generated.h"

UCLASS()
class BLACKHOLE_API UTankEnemyStateMachine : public UEnemyStateMachine
{
    GENERATED_BODY()

protected:
    virtual void InitializeStates() override;
    virtual void CreateDefaultStates() override;
    
    // Tank specific behavior
    virtual void BeginPlay() override;
    
private:
    void SetupTankParameters();
};