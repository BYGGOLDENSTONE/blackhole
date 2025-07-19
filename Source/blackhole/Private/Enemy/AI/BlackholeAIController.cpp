#include "Enemy/AI/BlackholeAIController.h"
#include "Enemy/BaseEnemy.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABlackholeAIController::ABlackholeAIController()
{
    // Enable movement and rotation updates
    bSetControlRotationFromPawnOrientation = false;
    
    // Path following component is created automatically by AAIController
}

void ABlackholeAIController::BeginPlay()
{
    Super::BeginPlay();
    
    ConfigureMovement();
}

void ABlackholeAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    
    if (!InPawn) return;
    
    // Configure movement for possessed enemy
    if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(InPawn))
    {
        // UE_LOG(LogTemp, Warning, TEXT("AIController possessed enemy: %s"), *Enemy->GetName());
    }
}

void ABlackholeAIController::OnUnPossess()
{
    Super::OnUnPossess();
}

void ABlackholeAIController::ConfigureMovement()
{
    // Configure movement to allow rotation while moving
    if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetPawn()))
    {
        if (UCharacterMovementComponent* MovementComp = Enemy->GetCharacterMovement())
        {
            // Allow rotation while moving
            MovementComp->bOrientRotationToMovement = false;
            MovementComp->bUseControllerDesiredRotation = false;
            MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
            
            // Log current movement settings
            // UE_LOG(LogTemp, Warning, TEXT("%s AIController: Movement configured - MaxWalkSpeed: %.0f, MovementMode: %d"),
            //     *Enemy->GetName(),
            //     MovementComp->MaxWalkSpeed,
            //     (int32)MovementComp->MovementMode);
            
            // Ensure movement is enabled
            if (MovementComp->MovementMode != MOVE_Walking)
            {
                MovementComp->SetMovementMode(MOVE_Walking);
                // UE_LOG(LogTemp, Warning, TEXT("%s AIController: Setting movement mode to Walking"), *Enemy->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s AIController: No movement component found!"), *Enemy->GetName());
        }
    }
}