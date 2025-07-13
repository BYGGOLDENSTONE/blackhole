#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "CombatEnemyStateMachine.generated.h"

UCLASS()
class BLACKHOLE_API UCombatEnemyStateMachine : public UEnemyStateMachine
{
    GENERATED_BODY()

protected:
    virtual void InitializeStates() override;
    virtual void CreateDefaultStates() override;
    
    // Combat enemy specific behavior
    virtual void BeginPlay() override;
    
private:
    void SetupCombatParameters();
};