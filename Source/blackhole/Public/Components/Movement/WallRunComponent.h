#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "WallRunComponent.generated.h"

class ABlackholePlayerCharacter;
class UCharacterMovementComponent;
class UCameraComponent;

UENUM(BlueprintType)
enum class EWallRunState : uint8
{
    None UMETA(DisplayName = "Not Wall Running"),
    Detecting UMETA(DisplayName = "Detecting Wall"),
    WallRunning UMETA(DisplayName = "Wall Running"),
    Falling UMETA(DisplayName = "Falling"),
    CoyoteTime UMETA(DisplayName = "Coyote Time Window")
};

UENUM(BlueprintType)
enum class EWallSide : uint8
{
    None UMETA(DisplayName = "No Wall"),
    Left UMETA(DisplayName = "Left Wall"),
    Right UMETA(DisplayName = "Right Wall")
};

USTRUCT(BlueprintType)
struct FWallRunSettings
{
    GENERATED_BODY()

    // Detection settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "50", ClampMax = "300"))
    float WallDetectionDistance = 45.0f;  // Further reduced - player runs very close to walls

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "20", ClampMax = "60"))
    float WallDetectionRadius = 20.0f;  // Further reduced - more precise detection

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "60", ClampMax = "85"))
    float MinWallAngle = 70.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "85", ClampMax = "120"))
    float MaxWallAngle = 110.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "100", ClampMax = "500"))
    float MinWallHeight = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "100", ClampMax = "1000", DisplayName = "Min Height From Ground"))
    float MinHeightFromGround = 150.0f;

    // Movement settings
    // Timer removed - wall run continues as long as speed is maintained
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "1.0", ClampMax = "10.0"))
    // float MaxWallRunDuration = 3.0f;

    // Speed boost multiplier removed - now preserves full dash speed
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "1.0", ClampMax = "1.5"))
    // float SpeedBoostMultiplier = 1.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "200", ClampMax = "800"))
    float WallRunSpeed = 600.0f;

    // Speed decay removed - player maintains max speed always
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    // float SpeedDecayRate = 1.2f;

    // Grace periods
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grace Periods", meta = (ClampMin = "0.05", ClampMax = "0.5"))
    float CoyoteTimeDuration = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grace Periods", meta = (ClampMin = "0.1", ClampMax = "0.5"))
    float CornerToleranceDuration = 0.2f;

    // Exit settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit", meta = (ClampMin = "100", ClampMax = "1000"))
    float LaunchImpulseStrength = 550.0f;  // Increased from 400 for better wall clearance

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit", meta = (ClampMin = "0.2", ClampMax = "2.0"))
    float WallJumpCooldown = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    float WallRunRestartCooldown = 1.5f;  // Prevent immediate wall run restart after ending

    // Visual settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual", meta = (ClampMin = "0", ClampMax = "45"))
    float CameraTiltAngle = 20.0f;  // Increased from 15 for more dramatic effect

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float CameraTiltSpeed = 1.0f;

    FWallRunSettings()
    {
        // Default values already set above
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallRunStateChanged, EWallRunState, NewState);
// Timer removed - wall run is unlimited based on speed
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallRunTimerUpdate, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWallDetected, EWallSide, Side, FVector, WallNormal);

/**
 * Enhanced Wall Run Component with momentum preservation, coyote time, and corner tolerance
 * Integrates with existing ability system using Gameplay Tags
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UWallRunComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWallRunComponent();

    // Component lifecycle
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Public interface
    UFUNCTION(BlueprintCallable, Category = "Wall Run")
    bool CanStartWallRun() const;

    UFUNCTION(BlueprintCallable, Category = "Wall Run")
    bool TryStartWallRun();

    UFUNCTION(BlueprintCallable, Category = "Wall Run")
    void EndWallRun(bool bJumpCancel = false);

    UFUNCTION(BlueprintCallable, Category = "Wall Run")
    void OnJumpPressed();

    UFUNCTION(BlueprintCallable, Category = "Wall Run")
    bool IsWallRunning() const { return CurrentState == EWallRunState::WallRunning; }

    UFUNCTION(BlueprintCallable, Category = "Wall Run")
    bool CanUseAbilityDuringWallRun(const class UAbilityComponent* Ability) const;

    UFUNCTION(BlueprintCallable, Category = "Wall Run")
    float GetWallRunTimeRemaining() const;

    UFUNCTION(BlueprintCallable, Category = "Wall Run")
    EWallSide GetCurrentWallSide() const { return CurrentWallSide; }

    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run Settings")
    FWallRunSettings Settings;

    // Note: This project uses a custom ability system
    // Wall run restrictions are handled through IsBasicAbility() checks

    // Visual effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    UParticleSystem* WallRunParticle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    USoundBase* WallRunStartSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    USoundBase* WallRunLoopSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    USoundBase* WallRunEndSound = nullptr;

    // Debug settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugVisuals = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugLogs = false;

    // Delegates
    UPROPERTY(BlueprintAssignable, Category = "Wall Run Events")
    FOnWallRunStateChanged OnWallRunStateChanged;

    // Timer removed - wall run is unlimited based on speed
    // UPROPERTY(BlueprintAssignable, Category = "Wall Run Events")
    // FOnWallRunTimerUpdate OnWallRunTimerUpdate;

    UPROPERTY(BlueprintAssignable, Category = "Wall Run Events")
    FOnWallDetected OnWallDetected;

protected:
    // Core state
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    EWallRunState CurrentState = EWallRunState::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    EWallSide CurrentWallSide = EWallSide::None;

    // Timers
    // float WallRunTimer = 0.0f; // Removed - wall run is unlimited
    float CoyoteTimer = 0.0f;
    float CornerToleranceTimer = 0.0f;
    float LastWallJumpTime = 0.0f;
    float LastWallRunEndTime = 0.0f; // Prevent immediate re-attachment to walls
    
    // Wall verification timer
    FTimerHandle WallVerificationTimer;

    // Wall data
    FVector CurrentWallNormal = FVector::ZeroVector;
    FVector LastValidWallNormal = FVector::ZeroVector;
    FVector WallRunDirection = FVector::ZeroVector;
    float CurrentWallRunSpeed = 0.0f;
    float WallRunStartHeight = 0.0f; // Store starting height to maintain consistent level

    // Cached components
    UPROPERTY()
    ABlackholePlayerCharacter* OwnerCharacter = nullptr;

    UPROPERTY()
    UCharacterMovementComponent* MovementComponent = nullptr;

    UPROPERTY()
    UCameraComponent* CameraComponent = nullptr;

    // Original values for restoration
    float OriginalGravityScale = 1.0f;
    float OriginalCameraRoll = 0.0f;
    
    // Particle component for wall run effect
    UPROPERTY()
    class UParticleSystemComponent* WallRunParticleComponent = nullptr;

    // Audio component for looping sound
    UPROPERTY()
    class UAudioComponent* WallRunAudioComponent = nullptr;

    // Internal functions
    void InitializeComponent();
    void UpdateStateMachine(float DeltaTime);
    void UpdateWallDetection(float DeltaTime);
    void UpdateWallRunMovement(float DeltaTime);
    void UpdateCoyoteTime(float DeltaTime);
    void UpdateCornerTolerance(float DeltaTime);
    void UpdateCameraTilt(float DeltaTime);
    void UpdateVisualEffects(float DeltaTime);

    // Detection functions
    bool DetectWall(FVector& OutWallNormal, EWallSide& OutWallSide) const;
    bool IsValidWallSurface(const FHitResult& Hit) const;
    bool CheckWallHeight(const FVector& WallLocation, const FVector& WallNormal) const;
    FVector CalculateWallRunDirection(const FVector& WallNormal, EWallSide WallSide) const;

    // Movement functions
    void ApplyWallRunMovement(float DeltaTime);
    void ApplyWallRunPhysics();
    void RestoreNormalMovement();
    FVector CalculateLaunchVelocity(bool bJumpCancel) const;
    
    // Wall jump functions
    void ExecuteWallJump();
    FVector CalculateWallJumpVelocity() const;
    
    // Wall verification
    void VerifyWallPresence();
    void StartWallVerificationTimer();
    void StopWallVerificationTimer();

    // State transitions
    void ChangeState(EWallRunState NewState);
    void StartWallRunInternal(const FVector& WallNormal, EWallSide WallSide);
    void EndWallRunInternal(bool bJumpCancel);

    // Ability system integration
    void ApplyWallRunGameplayTags();
    void RemoveWallRunGameplayTags();
    bool HasForwardInput() const;
    bool HasJumpInput() const;

    // Visual and audio
    void StartVisualEffects();
    void StopVisualEffects();
    void UpdateParticleEffects();
    void PlayWallRunSound(USoundBase* Sound);

    // Debug
    void DrawDebugInfo() const;
    void LogWallRunInfo(const FString& Message) const;
};