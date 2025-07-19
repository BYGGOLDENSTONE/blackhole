#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GravityDirectionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGravityDirectionChanged, FVector, NewGravityDirection);

UENUM(BlueprintType)
enum class EGravityAxis : uint8
{
    Default     UMETA(DisplayName = "Default (Down)"),
    Left        UMETA(DisplayName = "Left Wall"),
    Right       UMETA(DisplayName = "Right Wall"),
    Forward     UMETA(DisplayName = "Forward Wall"),
    Backward    UMETA(DisplayName = "Backward Wall"),
    Up          UMETA(DisplayName = "Ceiling"),
    Custom      UMETA(DisplayName = "Custom Direction")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UGravityDirectionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGravityDirectionComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Set gravity to a predefined axis
    UFUNCTION(BlueprintCallable, Category = "Gravity")
    void SetGravityAxis(EGravityAxis NewAxis);

    // Set gravity to a custom direction
    UFUNCTION(BlueprintCallable, Category = "Gravity")
    void SetGravityDirection(FVector NewDirection);

    // Smoothly transition to new gravity over time
    UFUNCTION(BlueprintCallable, Category = "Gravity")
    void TransitionToGravityDirection(FVector NewDirection, float TransitionTime = 2.0f);

    // Get current gravity direction
    UFUNCTION(BlueprintPure, Category = "Gravity")
    FVector GetGravityDirection() const { return CurrentGravityDirection; }

    // Get current gravity axis
    UFUNCTION(BlueprintPure, Category = "Gravity")
    EGravityAxis GetGravityAxis() const { return CurrentGravityAxis; }

    // Check if transitioning
    UFUNCTION(BlueprintPure, Category = "Gravity")
    bool IsTransitioning() const { return bIsTransitioning; }

    // Apply gravity shift to all characters in radius
    UFUNCTION(BlueprintCallable, Category = "Gravity")
    void ApplyGravityShiftInRadius(FVector Center, float Radius);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Gravity")
    FOnGravityDirectionChanged OnGravityDirectionChanged;

protected:
    // Update character movement for new gravity
    void UpdateCharacterMovement();
    
    // Apply gravity to a specific character
    void ApplyGravityToCharacter(class ACharacter* Character);

    // Calculate rotation for new gravity direction
    FRotator CalculateNewRotation(FVector OldGravity, FVector NewGravity);

private:
    // Current gravity direction (normalized)
    UPROPERTY(VisibleAnywhere, Category = "Gravity")
    FVector CurrentGravityDirection;

    // Target gravity direction for transitions
    UPROPERTY(VisibleAnywhere, Category = "Gravity")
    FVector TargetGravityDirection;

    // Current gravity axis
    UPROPERTY(VisibleAnywhere, Category = "Gravity")
    EGravityAxis CurrentGravityAxis;

    // Transition state
    UPROPERTY(VisibleAnywhere, Category = "Gravity")
    bool bIsTransitioning;

    UPROPERTY(VisibleAnywhere, Category = "Gravity")
    float TransitionTimeRemaining;

    UPROPERTY(VisibleAnywhere, Category = "Gravity")
    float TotalTransitionTime;

    // Gravity strength
    UPROPERTY(EditAnywhere, Category = "Gravity", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float GravityScale = 1.0f;

    // Should affect all characters or just owner?
    UPROPERTY(EditAnywhere, Category = "Gravity")
    bool bAffectsAllCharacters = false;

    // Smooth rotation during transition
    UPROPERTY(EditAnywhere, Category = "Gravity")
    bool bSmoothRotation = true;

    // Cached references
    UPROPERTY()
    class ACharacter* OwnerCharacter;

    UPROPERTY()
    class UCharacterMovementComponent* OwnerMovement;

    // Store original gravity
    float OriginalGravityZ;
    
    // Store original movement speed
    float OriginalMaxWalkSpeed;
    
    // Store original rotation settings
    bool bOriginalOrientRotationToMovement;
    
    // Smooth rotation state
    UPROPERTY()
    FRotator TargetCharacterRotation;
    
    UPROPERTY()
    bool bIsRotating = false;
};