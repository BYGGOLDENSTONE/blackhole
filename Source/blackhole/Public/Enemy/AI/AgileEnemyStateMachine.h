#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "AgileEnemyStateMachine.generated.h"

UCLASS()
class BLACKHOLE_API UAgileEnemyStateMachine : public UEnemyStateMachine
{
    GENERATED_BODY()

protected:
    virtual void InitializeStates() override;
    virtual void CreateDefaultStates() override;
    
    // Agile specific behavior
    virtual void BeginPlay() override;
    
private:
    void SetupAgileParameters();
};