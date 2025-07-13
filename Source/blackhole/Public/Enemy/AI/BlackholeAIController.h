#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BlackholeAIController.generated.h"

UCLASS()
class BLACKHOLE_API ABlackholeAIController : public AAIController
{
    GENERATED_BODY()

public:
    ABlackholeAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    
    // Configure AI perception and navigation
    virtual void BeginPlay() override;
    
private:
    // Movement configuration
    void ConfigureMovement();
};