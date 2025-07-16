#pragma once

#include "CoreMinimal.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "MindMelderStateMachine.generated.h"

UCLASS()
class BLACKHOLE_API UMindMelderStateMachine : public UEnemyStateMachine
{
    GENERATED_BODY()

protected:
    virtual void InitializeStates() override;
    virtual void CreateDefaultStates() override;
    
    // Mind Melder specific behavior
    virtual void BeginPlay() override;
    
private:
    void SetupMindMelderParameters();
};