#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "HackerEnemyStateMachine.generated.h"

UCLASS()
class BLACKHOLE_API UHackerEnemyStateMachine : public UEnemyStateMachine
{
    GENERATED_BODY()

protected:
    virtual void InitializeStates() override;
    virtual void CreateDefaultStates() override;
    
    // Hacker specific behavior
    virtual void BeginPlay() override;
    
private:
    void SetupHackerParameters();
};