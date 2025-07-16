#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "StandardEnemyStateMachine.generated.h"

UCLASS()
class BLACKHOLE_API UStandardEnemyStateMachine : public UEnemyStateMachine
{
    GENERATED_BODY()

protected:
    virtual void InitializeStates() override;
    virtual void CreateDefaultStates() override;
    
    // Standard enemy specific behavior
    virtual void BeginPlay() override;
    
private:
    void SetupStandardParameters();
};