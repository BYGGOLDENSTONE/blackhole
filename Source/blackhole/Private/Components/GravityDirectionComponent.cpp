#include "Components/GravityDirectionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

UGravityDirectionComponent::UGravityDirectionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    
    // Default gravity direction is down
    CurrentGravityDirection = FVector(0, 0, -1);
    TargetGravityDirection = CurrentGravityDirection;
    CurrentGravityAxis = EGravityAxis::Default;
    
    bIsTransitioning = false;
    TransitionTimeRemaining = 0.0f;
    TotalTransitionTime = 4.0f;  // Default smooth transition
    
    // Initialize movement speed
    OriginalMaxWalkSpeed = 0.0f;
    bOriginalOrientRotationToMovement = true;
}

void UGravityDirectionComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache references
    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        OwnerMovement = OwnerCharacter->GetCharacterMovement();
        if (OwnerMovement)
        {
            // Store original gravity
            OriginalGravityZ = OwnerMovement->GetGravityZ();
            // Store original movement speed
            OriginalMaxWalkSpeed = OwnerMovement->MaxWalkSpeed;
            // Store original orient rotation setting
            bOriginalOrientRotationToMovement = OwnerMovement->bOrientRotationToMovement;
        }
    }
}

void UGravityDirectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Restore original settings
    if (OwnerMovement)
    {
        OwnerMovement->SetGravityDirection(FVector(0, 0, -1));
        OwnerMovement->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
        if (OriginalMaxWalkSpeed > 0.0f)
        {
            OwnerMovement->MaxWalkSpeed = OriginalMaxWalkSpeed;
        }
    }
    
    Super::EndPlay(EndPlayReason);
}

void UGravityDirectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Handle gravity direction transition
    if (bIsTransitioning && TransitionTimeRemaining > 0.0f)
    {
        TransitionTimeRemaining -= DeltaTime;
        
        if (TransitionTimeRemaining <= 0.0f)
        {
            // Transition complete
            CurrentGravityDirection = TargetGravityDirection;
            bIsTransitioning = false;
            TransitionTimeRemaining = 0.0f;
            
            // Restore original movement speed
            if (OwnerMovement && OriginalMaxWalkSpeed > 0.0f)
            {
                OwnerMovement->MaxWalkSpeed = OriginalMaxWalkSpeed;
            }
            
            UpdateCharacterMovement();
            OnGravityDirectionChanged.Broadcast(CurrentGravityDirection);
        }
        else
        {
            // Interpolate gravity direction with smooth easing
            float Alpha = 1.0f - (TransitionTimeRemaining / TotalTransitionTime);
            // Use smoothstep for smoother transition
            Alpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);
            
            FVector NewDirection = FMath::Lerp(CurrentGravityDirection, TargetGravityDirection, Alpha);
            NewDirection.Normalize();
            
            CurrentGravityDirection = NewDirection;
            UpdateCharacterMovement();
            
            // Reduce movement speed during transition for stability
            if (OwnerMovement && OriginalMaxWalkSpeed > 0.0f)
            {
                // Slow down to 40% speed at the midpoint of transition
                float SpeedMultiplier = 1.0f - (0.6f * FMath::Sin(Alpha * PI));
                OwnerMovement->MaxWalkSpeed = OriginalMaxWalkSpeed * SpeedMultiplier;
            }
            
            #if WITH_EDITOR
            // Debug visualization
            if (OwnerCharacter)
            {
                FVector CharLocation = OwnerCharacter->GetActorLocation();
                // Show both current and target gravity during transition
                DrawDebugLine(GetWorld(), CharLocation, CharLocation + (CurrentGravityDirection * 200.0f), 
                    FColor::Blue, false, -1.0f, 0, 5.0f);
                DrawDebugLine(GetWorld(), CharLocation, CharLocation + (TargetGravityDirection * 200.0f), 
                    FColor::Green, false, -1.0f, 0, 3.0f);
                
                // Show transition progress
                FString ProgressText = FString::Printf(TEXT("Gravity Shift: %.1f%%"), Alpha * 100.0f);
                DrawDebugString(GetWorld(), CharLocation + FVector(0, 0, 100), ProgressText, nullptr, FColor::White, -1.0f);
            }
            #endif
        }
    }
    
    // Handle smooth rotation
    if (bIsRotating && OwnerCharacter)
    {
        FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
        // Slower rotation for smoother feel
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetCharacterRotation, DeltaTime, 2.0f);
        OwnerCharacter->SetActorRotation(NewRotation);
        
        // Don't update controller rotation - let player maintain camera control
        
        // Check if rotation is complete
        if (CurrentRotation.Equals(TargetCharacterRotation, 1.0f))
        {
            bIsRotating = false;
        }
    }
}

void UGravityDirectionComponent::SetGravityAxis(EGravityAxis NewAxis)
{
    FVector NewDirection;
    
    switch (NewAxis)
    {
        case EGravityAxis::Default:
            NewDirection = FVector(0, 0, -1);
            break;
        case EGravityAxis::Left:
            NewDirection = FVector(-1, 0, 0);
            break;
        case EGravityAxis::Right:
            NewDirection = FVector(1, 0, 0);
            break;
        case EGravityAxis::Forward:
            NewDirection = FVector(0, 1, 0);
            break;
        case EGravityAxis::Backward:
            NewDirection = FVector(0, -1, 0);
            break;
        case EGravityAxis::Up:
            NewDirection = FVector(0, 0, 1);
            break;
        default:
            return;
    }
    
    CurrentGravityAxis = NewAxis;
    SetGravityDirection(NewDirection);
}

void UGravityDirectionComponent::SetGravityDirection(FVector NewDirection)
{
    NewDirection.Normalize();
    if (NewDirection.IsZero()) return;
    
    // Always use smooth transition
    TransitionToGravityDirection(NewDirection, TotalTransitionTime);
}

void UGravityDirectionComponent::TransitionToGravityDirection(FVector NewDirection, float TransitionTime)
{
    NewDirection.Normalize();
    if (NewDirection.IsZero()) return;
    
    TargetGravityDirection = NewDirection;
    TotalTransitionTime = FMath::Max(0.1f, TransitionTime);
    TransitionTimeRemaining = TotalTransitionTime;
    bIsTransitioning = true;
    
    // Determine which axis we're transitioning to
    float DotX = FMath::Abs(NewDirection.X);
    float DotY = FMath::Abs(NewDirection.Y);
    float DotZ = FMath::Abs(NewDirection.Z);
    
    if (DotZ > DotX && DotZ > DotY)
    {
        CurrentGravityAxis = (NewDirection.Z < 0) ? EGravityAxis::Default : EGravityAxis::Up;
    }
    else if (DotX > DotY)
    {
        CurrentGravityAxis = (NewDirection.X < 0) ? EGravityAxis::Left : EGravityAxis::Right;
    }
    else
    {
        CurrentGravityAxis = (NewDirection.Y < 0) ? EGravityAxis::Backward : EGravityAxis::Forward;
    }
}

void UGravityDirectionComponent::UpdateCharacterMovement()
{
    if (!OwnerMovement || !OwnerCharacter) return;
    
    // Set custom gravity direction
    OwnerMovement->SetGravityDirection(CurrentGravityDirection);
    
    // Calculate new up vector (opposite of gravity)
    FVector NewUpVector = -CurrentGravityDirection;
    
    // Get current forward and right vectors
    FVector OldForward = OwnerCharacter->GetActorForwardVector();
    FVector OldRight = OwnerCharacter->GetActorRightVector();
    FVector OldUp = OwnerCharacter->GetActorUpVector();
    
    // Calculate rotation needed to align with new gravity
    FQuat RotationQuat = FQuat::FindBetweenNormals(OldUp, NewUpVector);
    
    // Apply rotation to forward and right vectors
    FVector NewForward = RotationQuat.RotateVector(OldForward);
    FVector NewRight = RotationQuat.RotateVector(OldRight);
    
    // Ensure vectors are orthogonal
    NewForward = (NewForward - (NewForward | NewUpVector) * NewUpVector).GetSafeNormal();
    NewRight = FVector::CrossProduct(NewUpVector, NewForward).GetSafeNormal();
    NewForward = FVector::CrossProduct(NewRight, NewUpVector).GetSafeNormal();
    
    // Create rotation from new coordinate system
    FMatrix RotationMatrix;
    RotationMatrix.SetAxis(0, NewForward);
    RotationMatrix.SetAxis(1, NewRight);
    RotationMatrix.SetAxis(2, NewUpVector);
    FRotator NewRotation = RotationMatrix.Rotator();
    
    // Apply rotation
    if (bSmoothRotation)
    {
        // Store target rotation for smooth interpolation
        TargetCharacterRotation = NewRotation;
        bIsRotating = true;
    }
    else
    {
        OwnerCharacter->SetActorRotation(NewRotation);
    }
    
    // Don't automatically update controller rotation - let player maintain camera control
    // The movement system already handles gravity-relative movement properly
    
    // Update movement component settings for new gravity
    OwnerMovement->bOrientRotationToMovement = true; // Let movement system handle rotation
    OwnerMovement->SetPlaneConstraintNormal(FVector::ZeroVector); // Remove any plane constraints
    
    // Smoothly adjust velocity to new gravity direction
    if (bIsTransitioning && !CurrentGravityDirection.Equals(TargetGravityDirection, 0.1f))
    {
        FVector CurrentVelocity = OwnerMovement->Velocity;
        float VelocityMagnitude = CurrentVelocity.Size();
        
        if (VelocityMagnitude > 0.1f)
        {
            // Project velocity onto new movement plane
            FVector VelocityDir = CurrentVelocity.GetSafeNormal();
            
            // Remove component parallel to new up vector
            VelocityDir = (VelocityDir - (VelocityDir | NewUpVector) * NewUpVector).GetSafeNormal();
            
            // Apply adjusted velocity with slight reduction for stability
            OwnerMovement->Velocity = VelocityDir * VelocityMagnitude * 0.7f;
        }
    }
    
    // Apply to all characters if enabled
    if (bAffectsAllCharacters)
    {
        ApplyGravityShiftInRadius(OwnerCharacter->GetActorLocation(), 10000.0f);
    }
}

void UGravityDirectionComponent::ApplyGravityToCharacter(ACharacter* Character)
{
    if (!Character) return;
    
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    if (!Movement) return;
    
    // Set gravity direction
    Movement->SetGravityDirection(CurrentGravityDirection);
    
    // Update character rotation
    if (bSmoothRotation)
    {
        FVector UpVector = -CurrentGravityDirection;
        FVector ForwardVector = Character->GetActorForwardVector();
        
        // Remove the component of forward that's parallel to up
        ForwardVector = ForwardVector - (ForwardVector | UpVector) * UpVector;
        ForwardVector.Normalize();
        
        if (!ForwardVector.IsZero())
        {
            FRotator NewRotation = FRotationMatrix::MakeFromZX(UpVector, ForwardVector).Rotator();
            Character->SetActorRotation(NewRotation);
        }
    }
}

void UGravityDirectionComponent::ApplyGravityShiftInRadius(FVector Center, float Radius)
{
    // Get all characters in radius
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        if (!Actor || Actor == GetOwner()) continue;
        
        float Distance = FVector::Dist(Center, Actor->GetActorLocation());
        if (Distance <= Radius)
        {
            if (ACharacter* Character = Cast<ACharacter>(Actor))
            {
                ApplyGravityToCharacter(Character);
            }
        }
    }
}

FRotator UGravityDirectionComponent::CalculateNewRotation(FVector OldGravity, FVector NewGravity)
{
    // Calculate the rotation needed to align with new gravity
    FVector OldUp = -OldGravity;
    FVector NewUp = -NewGravity;
    
    FQuat RotationQuat = FQuat::FindBetweenNormals(OldUp, NewUp);
    return RotationQuat.Rotator();
}