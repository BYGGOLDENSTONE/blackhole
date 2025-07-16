#include "Components/Movement/WallRunComponent.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "Systems/ResourceManager.h"
#include "Systems/ComboDetectionSubsystem.h"
#include "Engine/Engine.h"

UWallRunComponent::UWallRunComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    
    // Set tick group to post-physics to ensure we override velocity after movement component
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    
    // Disable debug logs by default (enable only when debugging)
    bShowDebugLogs = false;
    bShowDebugVisuals = false;
    
    // Camera effects are now set in header defaults
    // Settings.CameraTiltAngle = 20.0f (in header)
    // Settings.CameraTiltSpeed = 1.0f (in header)
}

void UWallRunComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeComponent();
}

void UWallRunComponent::InitializeComponent()
{
    // Cache owner character
    OwnerCharacter = Cast<ABlackholePlayerCharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        LogWallRunInfo("WallRunComponent: Owner is not a BlackholePlayerCharacter!");
        return;
    }
    
    // Cache movement component
    MovementComponent = OwnerCharacter->GetCharacterMovement();
    if (!MovementComponent)
    {
        LogWallRunInfo("WallRunComponent: No CharacterMovementComponent found!");
        return;
    }
    
    // Cache camera component
    CameraComponent = OwnerCharacter->GetCameraComponent();
    if (!CameraComponent)
    {
        LogWallRunInfo("WallRunComponent: No CameraComponent found!");
    }
    
    // Store original values
    OriginalGravityScale = MovementComponent->GravityScale;
    if (CameraComponent)
    {
        OriginalCameraRoll = CameraComponent->GetRelativeRotation().Roll;
    }
    
    // Ensure state is properly initialized
    CurrentState = EWallRunState::None;
    CurrentWallSide = EWallSide::None;
    LastWallRunEndTime = 0.0f; // Initialize cooldown timer

}

void UWallRunComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!OwnerCharacter || !MovementComponent)
    {
        return;
    }
    
    // Removed periodic tick logging - too verbose
    
    UpdateStateMachine(DeltaTime);
    UpdateCameraTilt(DeltaTime);
    UpdateVisualEffects(DeltaTime);
    
    if (bShowDebugVisuals)
    {
        DrawDebugInfo();
    }
}

void UWallRunComponent::UpdateStateMachine(float DeltaTime)
{
    switch (CurrentState)
    {
        case EWallRunState::None:
        {
            // Check if we can start wall running
            // Wall run requires player to be airborne (after jumping) AND high enough
            if (HasForwardInput() && !MovementComponent->IsMovingOnGround())
            {
                // Quick height check before attempting wall run
                bool bIsHighEnough = true;
                if (Settings.MinHeightFromGround > 0.0f)
                {
                    FVector PlayerLocation = OwnerCharacter->GetActorLocation();
                    float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                    FVector TraceStart = PlayerLocation - FVector(0, 0, CapsuleHalfHeight);
                    FVector TraceEnd = TraceStart - FVector(0, 0, Settings.MinHeightFromGround);
                    
                    FHitResult QuickGroundCheck;
                    FCollisionQueryParams QueryParams;
                    QueryParams.AddIgnoredActor(OwnerCharacter);
                    
                    if (GetWorld()->LineTraceSingleByChannel(QuickGroundCheck, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
                    {
                        bIsHighEnough = false; // Too close to ground
                    }
                }
                
                if (bIsHighEnough && CanStartWallRun() && TryStartWallRun())
                {
                    // State already changed in StartWallRunInternal
                }
                else if (bIsHighEnough && !MovementComponent->IsMovingOnGround())
                {
                    // Start coyote time if we're still airborne but couldn't start wall run
                    ChangeState(EWallRunState::CoyoteTime);
                    CoyoteTimer = 0.0f;
                }
            }
            break;
        }
        
        case EWallRunState::CoyoteTime:
        {
            UpdateCoyoteTime(DeltaTime);
            break;
        }
        
        case EWallRunState::WallRunning:
        {
            // Check if player is holding W key for walking animation
            if (!HasForwardInput())
            {
                // End wall run immediately when W is released
                EndWallRun(false);
                break;
            }
            
            UpdateWallRunMovement(DeltaTime);
            UpdateWallDetection(DeltaTime);
            
            // Remove speed check - we're controlling velocity directly now
            // Speed should remain constant as long as W is held
            break;
        }
        
        case EWallRunState::Detecting:
        {
            UpdateWallDetection(DeltaTime);
            break;
        }
        
        case EWallRunState::Falling:
        {
            UpdateCornerTolerance(DeltaTime);
            break;
        }
    }
}

void UWallRunComponent::UpdateCoyoteTime(float DeltaTime)
{
    CoyoteTimer += DeltaTime;
    
    // Show visual hint that wall run is available (if not on cooldown)
    if (GEngine && HasForwardInput())
    {
        FVector WallNormal;
        EWallSide WallSide;
        if (DetectWall(WallNormal, WallSide))
        {
            float CurrentTime = GetWorld()->GetTimeSeconds();
            float RestartCooldownRemaining = (LastWallRunEndTime + Settings.WallRunRestartCooldown) - CurrentTime;
            
            if (RestartCooldownRemaining <= 0.0f && IsLookingAtWall(WallNormal, WallSide))
            {
                // Debug message removed - wall run ready
            }
        }
    }
    
    // Try to start wall run during coyote time
    if (HasForwardInput() && TryStartWallRun())
    {
        // State already changed in StartWallRunInternal
        return;
    }
    
    // End coyote time
    if (CoyoteTimer >= Settings.CoyoteTimeDuration)
    {
        ChangeState(EWallRunState::None);
    }
}

void UWallRunComponent::UpdateWallDetection(float DeltaTime)
{
    FVector WallNormal;
    EWallSide WallSide;
    
    if (!DetectWall(WallNormal, WallSide))
    {
        if (CurrentState == EWallRunState::WallRunning)
        {
            // Check if player still has forward input before corner tolerance
            if (!HasForwardInput())
            {
                // No forward input - end wall run immediately
                EndWallRun(false);
                return;
            }
            
            // Lost wall contact but still has forward input - start corner tolerance
            ChangeState(EWallRunState::Falling);
            CornerToleranceTimer = 0.0f;
        }
        return;
    }
    
    // Update wall data
    CurrentWallNormal = WallNormal;
    CurrentWallSide = WallSide;
    LastValidWallNormal = WallNormal;
    WallRunDirection = CalculateWallRunDirection(WallNormal, WallSide);
    
    OnWallDetected.Broadcast(WallSide, WallNormal);
}

void UWallRunComponent::UpdateWallRunMovement(float DeltaTime)
{
    if (CurrentState != EWallRunState::WallRunning)
    {
        return;
    }
    
    // Clear any accumulated movement input to prevent interference
    if (MovementComponent)
    {
        MovementComponent->ConsumeInputVector();
    }
    
    ApplyWallRunMovement(DeltaTime);
}

void UWallRunComponent::UpdateCornerTolerance(float DeltaTime)
{
    CornerToleranceTimer += DeltaTime;
    
    // Try to detect a new wall during corner tolerance
    FVector WallNormal;
    EWallSide WallSide;
    
    if (DetectWall(WallNormal, WallSide))
    {
        // Found new wall - continue wall running
        CurrentWallNormal = WallNormal;
        CurrentWallSide = WallSide;
        WallRunDirection = CalculateWallRunDirection(WallNormal, WallSide);
        ChangeState(EWallRunState::WallRunning);
        return;
    }
    
    // End corner tolerance
    if (CornerToleranceTimer >= Settings.CornerToleranceDuration)
    {
        EndWallRun(false);
    }
}

bool UWallRunComponent::DetectWall(FVector& OutWallNormal, EWallSide& OutWallSide) const
{
    if (!OwnerCharacter || !MovementComponent)
    {
        return false;
    }
    
    FVector ActorLocation = OwnerCharacter->GetActorLocation();
    FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
    FVector RightVector = OwnerCharacter->GetActorRightVector();
    
    // Check both sides using capsule traces for better detection
    TArray<EWallSide> SidesToCheck = {EWallSide::Right, EWallSide::Left};
    
    for (EWallSide Side : SidesToCheck)
    {
        FVector SideVector = (Side == EWallSide::Right) ? RightVector : -RightVector;
        FVector TraceStart = ActorLocation;
        FVector TraceEnd = TraceStart + (SideVector * Settings.WallDetectionDistance);
        
        // Use capsule trace for better detection around corners
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(OwnerCharacter);
        QueryParams.bTraceComplex = false;
        
        bool bHit = GetWorld()->SweepSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            FQuat::Identity,
            ECC_WorldStatic,
            FCollisionShape::MakeCapsule(Settings.WallDetectionRadius, 50.0f),
            QueryParams
        );
        
        if (bHit && IsValidWallSurface(HitResult))
        {
            // Check wall height
            if (CheckWallHeight(HitResult.Location, HitResult.Normal))
            {
                OutWallNormal = HitResult.Normal;
                OutWallSide = Side;
                return true;
            }
        }
    }
    
    return false;
}

bool UWallRunComponent::IsValidWallSurface(const FHitResult& Hit) const
{
    // Check wall angle (should be roughly vertical)
    FVector WallNormal = Hit.Normal;
    float WallAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(WallNormal, FVector::UpVector)));
    
    // Allow straight vertical walls (90 degrees) by expanding the range
    if (WallAngle < 70.0f || WallAngle > 110.0f) // Changed from 30-75 to 70-110 to include 90° walls
    {
        return false;
    }
    
    // Check if wall is facing toward the player
    FVector ToPlayer = (OwnerCharacter->GetActorLocation() - Hit.Location).GetSafeNormal();
    float DotProduct = FVector::DotProduct(WallNormal, ToPlayer);
    
    return DotProduct > 0.1f; // Wall should be facing toward player
}

bool UWallRunComponent::CheckWallHeight(const FVector& WallLocation, const FVector& WallNormal) const
{
    // Trace upward from wall contact point to check wall height
    FVector TraceStart = WallLocation;
    FVector TraceEnd = TraceStart + (FVector::UpVector * Settings.MinWallHeight);
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    
    // If we don't hit anything going up, the wall is tall enough
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        ECC_WorldStatic,
        QueryParams
    );
    
    return !bHit; // No hit means wall is tall enough
}

bool UWallRunComponent::IsLookingAtWall(const FVector& WallNormal, EWallSide WallSide) const
{
    if (!OwnerCharacter || !CameraComponent)
    {
        return false;
    }
    
    // Get camera forward direction
    FVector CameraForward = CameraComponent->GetForwardVector();
    
    // Get direction to wall (opposite of wall normal)
    FVector ToWall = -WallNormal;
    
    // Calculate dot product to see if camera is facing the wall
    float DotProduct = FVector::DotProduct(CameraForward, ToWall);
    
    // Debug visualization
    if (bShowDebugVisuals && GetWorld())
    {
        FVector PlayerLocation = OwnerCharacter->GetActorLocation();
        DrawDebugLine(GetWorld(), PlayerLocation, PlayerLocation + CameraForward * 200.0f, 
            FColor::Green, false, 0.1f, 0, 2.0f);
        DrawDebugLine(GetWorld(), PlayerLocation, PlayerLocation + ToWall * 200.0f, 
            FColor::Blue, false, 0.1f, 0, 2.0f);
        DrawDebugString(GetWorld(), PlayerLocation + FVector(0, 0, 120), 
            FString::Printf(TEXT("Look Dot: %.2f (need %.2f)"), DotProduct, Settings.MinLookAtDotProduct), 
            nullptr, DotProduct >= Settings.MinLookAtDotProduct ? FColor::Green : FColor::Red, 0.1f);
    }
    
    return DotProduct >= Settings.MinLookAtDotProduct;
}

FVector UWallRunComponent::CalculateWallRunDirection(const FVector& WallNormal, EWallSide WallSide) const
{
    if (!OwnerCharacter)
    {
        return FVector::ZeroVector;
    }
    
    FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
    FVector WallForward;
    
    if (WallSide == EWallSide::Right)
    {
        // Cross product: Wall Normal x Up = Forward along wall (right side)
        WallForward = FVector::CrossProduct(WallNormal, FVector::UpVector).GetSafeNormal();
    }
    else
    {
        // Cross product: Up x Wall Normal = Forward along wall (left side)
        WallForward = FVector::CrossProduct(FVector::UpVector, WallNormal).GetSafeNormal();
    }
    
    // Choose direction based on player's current movement
    float DotProduct = FVector::DotProduct(ForwardVector, WallForward);
    if (DotProduct < 0)
    {
        WallForward = -WallForward;
    }
    
    return WallForward;
}

bool UWallRunComponent::CanStartWallRun() const
{
    if (!OwnerCharacter || !MovementComponent)
    {
        return false;
    }
    
    // Check if we're not already wall running
    if (CurrentState == EWallRunState::WallRunning)
    {
        return false;
    }
    
    // CRITICAL: Player must be airborne to start wall run
    if (MovementComponent->IsMovingOnGround())
    {
        // Show feedback when player tries to wall run from ground
        if (HasForwardInput() && GEngine)
        {
            // Only show message if there's a wall nearby
            FVector WallNormal;
            EWallSide WallSide;
            if (DetectWall(WallNormal, WallSide))
            {
                GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, 
                    TEXT("Jump first to wall run!"));
            }
        }
        return false; // Can't start wall run while on ground
    }
    
    // Check minimum height from ground requirement - CRITICAL for intentional wall runs
    if (Settings.MinHeightFromGround > 0.0f)
    {
        FVector PlayerLocation = OwnerCharacter->GetActorLocation();
        // Start trace from player's feet, not center
        float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        FVector TraceStart = PlayerLocation - FVector(0, 0, CapsuleHalfHeight);
        FVector TraceEnd = TraceStart - FVector(0, 0, Settings.MinHeightFromGround * 2.0f); // Trace down further
        
        FHitResult GroundHit;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(OwnerCharacter);
        
        // Draw debug line to visualize height check
        #if WITH_EDITOR
        if (bShowDebugVisuals)
        {
            DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 0.1f, 0, 2.0f);
        }
        #endif
        
        if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
        {
            float HeightFromGround = (TraceStart - GroundHit.Location).Z;
            
            #if WITH_EDITOR
            if (bShowDebugVisuals)
            {
                DrawDebugString(GetWorld(), GroundHit.Location, FString::Printf(TEXT("Height: %.1f"), HeightFromGround), 
                    nullptr, HeightFromGround >= Settings.MinHeightFromGround ? FColor::Green : FColor::Red, 0.1f);
            }
            #endif
            
            if (HeightFromGround < Settings.MinHeightFromGround)
            {
                // Show feedback to player when too low
                if (GEngine && HasForwardInput())
                {
                    FVector WallNormal;
                    EWallSide WallSide;
                    if (DetectWall(WallNormal, WallSide))
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, 
                            FString::Printf(TEXT("Too low for wall run! Need %.0f height (currently %.0f)"), 
                            Settings.MinHeightFromGround, HeightFromGround));
                    }
                }
                return false;
            }
        }
        else
        {
            // No ground found within trace distance - player is high enough
            UE_LOG(LogTemp, VeryVerbose, TEXT("WallRun: No ground found - height check passed"));
        }
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Check wall run restart cooldown (prevents immediate re-attachment after wall jump)
    float RestartCooldownRemaining = (LastWallRunEndTime + Settings.WallRunRestartCooldown) - CurrentTime;
    if (RestartCooldownRemaining > 0.0f)
    {
        if (GEngine && HasForwardInput())
        {
            // Show cooldown feedback occasionally
            static float LastCooldownMessageTime = 0.0f;
            if (CurrentTime - LastCooldownMessageTime > 0.5f)
            {
                // Debug message removed - wall run cooldown
                LastCooldownMessageTime = CurrentTime;
            }
        }
        return false;
    }
    
    // Check wall jump cooldown
    float JumpCooldownRemaining = (LastWallJumpTime + Settings.WallJumpCooldown) - CurrentTime;
    if (JumpCooldownRemaining > 0.0f)
    {
        // Removed cooldown message - shown too frequently
        return false;
    }
    
    // Check if player has forward input and sufficient speed
    if (!HasForwardInput())
    {
        return false;
    }
    
    float CurrentSpeed = MovementComponent->Velocity.Size2D();
    if (CurrentSpeed < 200.0f) // Minimum speed requirement
    {
        // Removed speed requirement message - shown too frequently
        return false;
    }
    
    // Check if player is looking at the wall
    FVector WallNormal;
    EWallSide WallSide;
    if (DetectWall(WallNormal, WallSide))
    {
        if (!IsLookingAtWall(WallNormal, WallSide))
        {
            // Show feedback when player is not looking at the wall
            if (HasForwardInput() && GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, 
                    TEXT("Look at the wall to start wall run!"));
            }
            return false;
        }
    }
    
    return true;
}

bool UWallRunComponent::TryStartWallRun()
{
    FVector WallNormal;
    EWallSide WallSide;
    
    if (!DetectWall(WallNormal, WallSide))
    {
        // Show feedback that no wall was found
        // Removed "No valid wall detected" message - too spammy
        return false;
    }
    
    StartWallRunInternal(WallNormal, WallSide);
    return true;
}

void UWallRunComponent::StartWallRunInternal(const FVector& WallNormal, EWallSide WallSide)
{
    // CRITICAL: Change state FIRST before doing anything else
    ChangeState(EWallRunState::WallRunning);
    
    // Store wall data
    CurrentWallNormal = WallNormal;
    CurrentWallSide = WallSide;
    LastValidWallNormal = WallNormal;
    WallRunDirection = CalculateWallRunDirection(WallNormal, WallSide);
    
    // Store starting height to maintain consistent level
    if (OwnerCharacter)
    {
        WallRunStartHeight = OwnerCharacter->GetActorLocation().Z;
    }
    
    // Calculate initial speed with momentum preservation
    float CurrentSpeed = MovementComponent->Velocity.Size2D();
    
    // If player has high speed (from dash), limit to 1000
    if (CurrentSpeed > Settings.WallRunSpeed)
    {
        // Player is dashing or moving fast - cap speed at 1000
        CurrentWallRunSpeed = FMath::Min(CurrentSpeed, 1000.0f);
        
        // Dash + Wall Run combo detected - reward WP
        if (CurrentSpeed > 2500.0f) // Likely from dash (dash speed is 3000)
        {
            if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
            {
                if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
                {
                    float WPReward = 10.0f; // Wall run combo reward
                    ResourceMgr->AddWillPower(WPReward);
                    UE_LOG(LogTemp, Warning, TEXT("Dash + Wall Run combo: Player gained +%.1f WP, Speed capped at 1000"), WPReward);
                }
            }
        }
    }
    else
    {
        // Normal speed wall run
        CurrentWallRunSpeed = Settings.WallRunSpeed;
    }
    
    // Register wall run input with combo system
    if (UComboDetectionSubsystem* ComboSystem = GetWorld()->GetSubsystem<UComboDetectionSubsystem>())
    {
        ComboSystem->RegisterInput(OwnerCharacter, EComboInput::WallRun, OwnerCharacter->GetActorLocation());
    }
    
    // Apply wall run physics
    ApplyWallRunPhysics();
    
    // Apply gameplay tags for ability restrictions
    ApplyWallRunGameplayTags();
    
    // Start visual and audio effects
    StartVisualEffects();
    
    // Start wall verification timer to check wall presence every second
    StartWallVerificationTimer();
    
    // Debug message removed - wall running instructions

}

void UWallRunComponent::ApplyWallRunPhysics()
{
    if (!MovementComponent)
    {
        return;
    }
    
    // Store original rotation settings
    bOriginalOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
    
    // Disable gravity
    MovementComponent->GravityScale = 0.0f;
    
    // Disable rotation to movement during wall run to allow free camera look
    MovementComponent->bOrientRotationToMovement = false;
    
    // Set movement mode to flying for better control
    MovementComponent->SetMovementMode(MOVE_Flying);
    
    // Apply initial wall run velocity
    ApplyWallRunMovement(0.0f);
}

void UWallRunComponent::ApplyWallRunMovement(float DeltaTime)
{
    if (!MovementComponent || CurrentState != EWallRunState::WallRunning || !OwnerCharacter)
    {
        return;
    }
    
    // NO SPEED DECAY - player maintains maximum speed always
    // CurrentWallRunSpeed remains constant throughout wall run
    
    // Calculate wall run velocity - always forward along wall at same height
    FVector WallRunVelocity = WallRunDirection * CurrentWallRunSpeed;
    
    // Maintain starting height - adjust Z velocity to keep at consistent level
    float CurrentHeight = OwnerCharacter->GetActorLocation().Z;
    float HeightDifference = WallRunStartHeight - CurrentHeight;
    
    // Apply gentle correction to maintain height (not too aggressive to avoid jitter)
    WallRunVelocity.Z = HeightDifference * 2.0f; // Multiply by 2 for gentle correction
    
    // Add slight velocity toward wall to keep player close to the wall
    FVector TowardWall = CurrentWallNormal * -50.0f; // Small push toward wall
    WallRunVelocity += TowardWall;
    
    // Apply velocity - camera direction doesn't affect movement
    MovementComponent->Velocity = WallRunVelocity;
}

void UWallRunComponent::EndWallRun(bool bJumpCancel)
{
    if (CurrentState != EWallRunState::WallRunning && CurrentState != EWallRunState::Falling)
    {
        return;
    }
    
    EndWallRunInternal(bJumpCancel);
}

void UWallRunComponent::EndWallRunInternal(bool bJumpCancel)
{
    // Calculate launch velocity if jump canceling
    FVector LaunchVelocity = FVector::ZeroVector;
    if (bJumpCancel)
    {
        LaunchVelocity = CalculateLaunchVelocity(true);
        LastWallJumpTime = GetWorld()->GetTimeSeconds();
    }
    
    // Restore normal movement
    RestoreNormalMovement();
    
    // Apply launch velocity
    if (bJumpCancel && MovementComponent)
    {
        MovementComponent->Velocity = LaunchVelocity;
    }
    
    // Remove gameplay tags
    RemoveWallRunGameplayTags();
    
    // Stop visual and audio effects
    StopVisualEffects();
    
    // Stop wall verification timer
    StopWallVerificationTimer();
    
    // Debug message removed - wall run end message
    
    // Set wall run end time for cooldown
    LastWallRunEndTime = GetWorld()->GetTimeSeconds();
    
    // Reset state
    ChangeState(EWallRunState::None);
    CurrentWallSide = EWallSide::None;
    CurrentWallNormal = FVector::ZeroVector;
    WallRunDirection = FVector::ZeroVector;
    WallRunStartHeight = 0.0f;
    
    // Debug message removed - wall jump cooldown
    
    OnWallRunStateChanged.Broadcast(EWallRunState::None);
}

void UWallRunComponent::RestoreNormalMovement()
{
    if (!MovementComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("WallRun: RestoreNormalMovement - MovementComponent is NULL!"));
        return;
    }
    
    UE_LOG(LogTemp, Error, TEXT("WallRun: Restoring gravity scale from %.2f to %.2f"), 
        MovementComponent->GravityScale, OriginalGravityScale);
    
    // Restore gravity
    MovementComponent->GravityScale = OriginalGravityScale;
    
    // Restore rotation to movement setting
    MovementComponent->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
    
    // Set to falling mode to allow the launch to work properly
    MovementComponent->SetMovementMode(MOVE_Falling);
    UE_LOG(LogTemp, Error, TEXT("WallRun: Gravity restored, movement mode set to FALLING for jump"));
    
    // If player had high speed (from dash), gradually reduce speed instead of immediate stop
    if (CurrentWallRunSpeed > Settings.WallRunSpeed * 1.5f)
    {
        // Apply momentum preservation for high-speed wall runs
        FVector CurrentVelocity = MovementComponent->Velocity;
        float CurrentSpeed = CurrentVelocity.Size2D();
        
        if (CurrentSpeed > Settings.WallRunSpeed)
        {
            // Preserve direction but reduce speed gradually
            FVector Direction = CurrentVelocity.GetSafeNormal2D();
            float TargetSpeed = FMath::Max(Settings.WallRunSpeed, CurrentSpeed * 0.7f);
            
            FVector NewVelocity = Direction * TargetSpeed;
            NewVelocity.Z = CurrentVelocity.Z; // Preserve vertical velocity
            
            MovementComponent->Velocity = NewVelocity;
            
            // Debug message removed - speed preserved
        }
    }
}

FVector UWallRunComponent::CalculateWallJumpVelocity() const
{
    if (!OwnerCharacter || !MovementComponent)
    {
        return FVector::ZeroVector;
    }
    
    // Create strong diagonal trajectory based on wall side
    FVector AwayFromWall, DiagonalDirection;
    
    if (CurrentWallSide == EWallSide::Right)
    {
        // Right wall: Jump LEFT and UP and FORWARD
        AwayFromWall = -OwnerCharacter->GetActorRightVector(); // Jump left
        DiagonalDirection = (AwayFromWall + FVector::UpVector + OwnerCharacter->GetActorForwardVector()).GetSafeNormal();
    }
    else // Left wall
    {
        // Left wall: Jump RIGHT and UP and FORWARD  
        AwayFromWall = OwnerCharacter->GetActorRightVector(); // Jump right
        DiagonalDirection = (AwayFromWall + FVector::UpVector + OwnerCharacter->GetActorForwardVector()).GetSafeNormal();
    }
    
    // Strong diagonal jump with increased power
    float JumpPower = Settings.LaunchImpulseStrength * 1.5f; // 50% more powerful
    FVector WallJumpVelocity = DiagonalDirection * JumpPower;
    
    return WallJumpVelocity;
}

FVector UWallRunComponent::CalculateLaunchVelocity(bool bJumpCancel) const
{
    if (!OwnerCharacter || !MovementComponent)
    {
        return FVector::ZeroVector;
    }
    
    FVector LaunchVelocity = FVector::ZeroVector;
    
    if (bJumpCancel)
    {
        // This shouldn't be used anymore - wall jumps use CalculateWallJumpVelocity()
        LaunchVelocity = CalculateWallJumpVelocity();
    }
    else
    {
        // Preserve momentum when wall run ends naturally
        if (CurrentWallRunSpeed > Settings.WallRunSpeed)
        {
            // High speed (from dash) - preserve more momentum
            LaunchVelocity = WallRunDirection * (CurrentWallRunSpeed * 0.8f);
        }
        else
        {
            // Normal speed - some forward momentum
            LaunchVelocity = WallRunDirection * (CurrentWallRunSpeed * 0.5f);
        }
    }
    
    return LaunchVelocity;
}

void UWallRunComponent::OnJumpPressed()
{
    UE_LOG(LogTemp, Error, TEXT("WallRun: OnJumpPressed() called! CurrentState = %d"), (int32)CurrentState);
    
    // Debug message removed - jump pressed state
    
    if (CurrentState == EWallRunState::WallRunning)
    {
        UE_LOG(LogTemp, Error, TEXT("WallRun: State is WallRunning, calling ExecuteWallJump()"));
        // Execute dedicated wall jump immediately
        ExecuteWallJump();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WallRun: NOT in WallRunning state, ignoring jump"));
    }
}

void UWallRunComponent::ExecuteWallJump()
{
    UE_LOG(LogTemp, Error, TEXT("WallRun: ExecuteWallJump() called!"));
    
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("WallRun: ExecuteWallJump() failed - OwnerCharacter is NULL"));
        return;
    }
    
    if (!MovementComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("WallRun: ExecuteWallJump() failed - MovementComponent is NULL"));
        return;
    }
    
    if (CurrentState != EWallRunState::WallRunning)
    {
        UE_LOG(LogTemp, Error, TEXT("WallRun: ExecuteWallJump() failed - CurrentState is %d, not WallRunning"), (int32)CurrentState);
        return;
    }
    
    UE_LOG(LogTemp, Error, TEXT("WallRun: ExecuteWallJump() - All checks passed, executing wall jump!"));
    
    // Store wall side before resetting it
    EWallSide JumpWallSide = CurrentWallSide;
    
    // Calculate diagonal wall jump velocity based on wall side
    FVector WallJumpVelocity = CalculateWallJumpVelocity();
    UE_LOG(LogTemp, Error, TEXT("WallRun: Calculated WallJumpVelocity = %s"), *WallJumpVelocity.ToString());
    
    // Set wall run end time for cooldown
    LastWallRunEndTime = GetWorld()->GetTimeSeconds();
    LastWallJumpTime = GetWorld()->GetTimeSeconds();
    
    // Debug message removed - wall jump feedback
    
    // Apply wall jump velocity FIRST before any state changes
    FVector VelocityBefore = MovementComponent->Velocity;
    UE_LOG(LogTemp, Error, TEXT("WallRun: Velocity BEFORE = %s"), *VelocityBefore.ToString());
    
    // Use Launch instead of directly setting velocity - this is more reliable
    UE_LOG(LogTemp, Error, TEXT("WallRun: Launching character with velocity = %s"), *WallJumpVelocity.ToString());
    MovementComponent->Launch(WallJumpVelocity);
    
    FVector VelocityAfter = MovementComponent->Velocity;
    UE_LOG(LogTemp, Error, TEXT("WallRun: Velocity AFTER Launch = %s"), *VelocityAfter.ToString());
    
    // Now end wall run state
    UE_LOG(LogTemp, Error, TEXT("WallRun: Changing state to None"));
    ChangeState(EWallRunState::None);
    CurrentWallSide = EWallSide::None;
    
    // Restore normal movement AFTER launching
    UE_LOG(LogTemp, Error, TEXT("WallRun: Restoring normal movement"));
    RestoreNormalMovement();
    
    // Remove gameplay tags
    RemoveWallRunGameplayTags();
    
    // Stop visual and audio effects
    StopVisualEffects();
    
    // Reset wall data
    CurrentWallNormal = FVector::ZeroVector;
    WallRunDirection = FVector::ZeroVector;
    WallRunStartHeight = 0.0f;
    
    OnWallRunStateChanged.Broadcast(EWallRunState::None);
    
    // Add a short delay to let the velocity take effect
    FTimerHandle DelayHandle;
    GetWorld()->GetTimerManager().SetTimer(DelayHandle, [this]()
    {
        if (MovementComponent)
        {
            FVector CurrentVel = MovementComponent->Velocity;
            UE_LOG(LogTemp, Error, TEXT("WallRun: Velocity after 0.1s delay = %s"), *CurrentVel.ToString());
        }
    }, 0.1f, false);
}

bool UWallRunComponent::CanUseAbilityDuringWallRun(const UAbilityComponent* Ability) const
{
    if (!Ability)
    {
        return true; // Safety check
    }
    
    // Always allow abilities when not wall running
    if (CurrentState != EWallRunState::WallRunning)
    {
        return true;
    }
    
    // Get the ability class name for more reliable detection
    FString AbilityClassName = Ability->GetClass()->GetName();
    
    // SPECIAL CASE: Block HackerJumpAbility during wall run - use dedicated wall jump instead
    if (AbilityClassName.Contains(TEXT("HackerJumpAbility")))
    {
        // Debug message removed - wall jump active
        return false; // Block regular jump, use wall jump instead
    }
    
    // SPECIAL CASE: Always block HackerDashAbility during wall run
    if (AbilityClassName.Contains(TEXT("HackerDashAbility")))
    {
        // Debug message removed - dash blocked during wall run
        return false;
    }
    
    // Block other basic abilities (except jump which we handled above)
    if (Ability->IsBasicAbility())
    {
        return false;
    }
    
    // Allow all advanced abilities
    return true;
}

void UWallRunComponent::ApplyWallRunGameplayTags()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Note: This project uses a custom ability system, not Unreal's GAS
    // The wall run restrictions are handled through CanUseAbilityDuringWallRun() instead
}

void UWallRunComponent::RemoveWallRunGameplayTags()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Note: This project uses a custom ability system, not Unreal's GAS
    // The wall run restrictions are handled through CanUseAbilityDuringWallRun() instead
}

float UWallRunComponent::GetWallRunTimeRemaining() const
{
    // Timer removed - return large value to indicate unlimited time
    if (CurrentState != EWallRunState::WallRunning)
    {
        return 0.0f;
    }
    
    return 999.0f; // Unlimited wall run time
}

void UWallRunComponent::UpdateCameraTilt(float DeltaTime)
{
    if (!CameraComponent || Settings.CameraTiltAngle <= 0.0f)
    {
        return;
    }
    
    float TargetRoll = OriginalCameraRoll;
    
    if (CurrentState == EWallRunState::WallRunning)
    {
        // Tilt camera based on wall side
        TargetRoll = (CurrentWallSide == EWallSide::Right) ? 
            -Settings.CameraTiltAngle : Settings.CameraTiltAngle;
    }
    
    // Smoothly interpolate to target roll
    FRotator CurrentRotation = CameraComponent->GetRelativeRotation();
    float NewRoll = FMath::FInterpTo(CurrentRotation.Roll, TargetRoll, 
        DeltaTime, Settings.CameraTiltSpeed);
    
    CameraComponent->SetRelativeRotation(FRotator(CurrentRotation.Pitch, CurrentRotation.Yaw, NewRoll));
}

void UWallRunComponent::StartVisualEffects()
{
    // Start particle effect
    if (WallRunParticle && OwnerCharacter)
    {
        FVector SpawnLocation = OwnerCharacter->GetActorLocation();
        WallRunParticleComponent = UGameplayStatics::SpawnEmitterAttached(
            WallRunParticle,
            OwnerCharacter->GetRootComponent(),
            NAME_None,
            SpawnLocation,
            FRotator::ZeroRotator,
            EAttachLocation::KeepWorldPosition
        );
    }
    
    // Start audio effect
    if (WallRunStartSound)
    {
        PlayWallRunSound(WallRunStartSound);
    }
    
    if (WallRunLoopSound && OwnerCharacter)
    {
        WallRunAudioComponent = UGameplayStatics::SpawnSoundAttached(
            WallRunLoopSound,
            OwnerCharacter->GetRootComponent()
        );
    }
}

void UWallRunComponent::StopVisualEffects()
{
    // Stop particle effect
    if (WallRunParticleComponent)
    {
        WallRunParticleComponent->DestroyComponent();
        WallRunParticleComponent = nullptr;
    }
    
    // Stop audio effect
    if (WallRunAudioComponent)
    {
        WallRunAudioComponent->Stop();
        WallRunAudioComponent = nullptr;
    }
    
    if (WallRunEndSound)
    {
        PlayWallRunSound(WallRunEndSound);
    }
}

void UWallRunComponent::UpdateVisualEffects(float DeltaTime)
{
    UpdateParticleEffects();
}

void UWallRunComponent::UpdateParticleEffects()
{
    if (WallRunParticleComponent && CurrentState == EWallRunState::WallRunning)
    {
        // Update particle location to follow wall contact point
        FVector ContactPoint = OwnerCharacter->GetActorLocation() + 
            (CurrentWallSide == EWallSide::Right ? 
                OwnerCharacter->GetActorRightVector() : 
                -OwnerCharacter->GetActorRightVector()) * 80.0f;
        
        WallRunParticleComponent->SetWorldLocation(ContactPoint);
    }
}

void UWallRunComponent::PlayWallRunSound(USoundBase* Sound)
{
    if (Sound && OwnerCharacter)
    {
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            Sound,
            OwnerCharacter->GetActorLocation()
        );
    }
}

void UWallRunComponent::ChangeState(EWallRunState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }
    
    EWallRunState OldState = CurrentState;
    CurrentState = NewState;
    
    FString StateNames[] = {TEXT("None"), TEXT("Detecting"), TEXT("WallRunning"), TEXT("Falling"), TEXT("CoyoteTime")};
    FString OldStateName = (int32)OldState < 5 ? StateNames[(int32)OldState] : TEXT("Unknown");
    FString NewStateName = (int32)NewState < 5 ? StateNames[(int32)NewState] : TEXT("Unknown");
    
    if (bShowDebugLogs)
    {
        LogWallRunInfo(FString::Printf(TEXT("STATE CHANGED: %s -> %s"), *OldStateName, *NewStateName));
    }
    
    OnWallRunStateChanged.Broadcast(NewState);
    
    // Notify jump ability about wall run state change
    if (OwnerCharacter)
    {
        if (UHackerJumpAbility* JumpAbility = OwnerCharacter->FindComponentByClass<UHackerJumpAbility>())
        {
            bool bIsWallRunning = (NewState == EWallRunState::WallRunning);
            JumpAbility->OnWallRunStateChanged(bIsWallRunning);
        }
    }
}

bool UWallRunComponent::HasForwardInput() const
{
    if (!OwnerCharacter)
    {
        return false;
    }
    
    // Get raw input vector
    FVector InputVector = OwnerCharacter->GetLastMovementInputVector();
    
    // If we're already wall running, only check if there's any forward component in the input
    // This allows player to look around while holding W
    if (CurrentState == EWallRunState::WallRunning)
    {
        // Check if there's any forward input component (W key)
        // We check the Y component of the 2D input which represents forward/back
        // This is independent of camera direction
        return InputVector.Size() > 0.1f; // Any movement input continues wall run
    }
    
    // For starting wall run, check if input is somewhat forward-facing
    FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
    float ForwardInput = FVector::DotProduct(InputVector, ForwardVector);
    return ForwardInput > 0.1f; // Need forward input to start wall run
}

bool UWallRunComponent::HasJumpInput() const
{
    if (!OwnerCharacter)
    {
        return false;
    }
    
    // This is handled via OnJumpPressed() callback from the player character
    // We don't need to check jump input here for wall run initiation
    return false;
}

void UWallRunComponent::DrawDebugInfo() const
{
    if (!OwnerCharacter || !GetWorld())
    {
        return;
    }
    
    FVector ActorLocation = OwnerCharacter->GetActorLocation();
    FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
    FVector RightVector = OwnerCharacter->GetActorRightVector();
    
    // Draw detection ranges
    FVector RightTraceEnd = ActorLocation + (RightVector * Settings.WallDetectionDistance);
    FVector LeftTraceEnd = ActorLocation + (-RightVector * Settings.WallDetectionDistance);
    
    DrawDebugCapsule(GetWorld(), RightTraceEnd, 50.0f, Settings.WallDetectionRadius, 
        FQuat::Identity, FColor::Yellow, false, 0.0f, 0, 2.0f);
    DrawDebugCapsule(GetWorld(), LeftTraceEnd, 50.0f, Settings.WallDetectionRadius, 
        FQuat::Identity, FColor::Yellow, false, 0.0f, 0, 2.0f);
    
    // Draw current wall normal if wall running
    if (CurrentState == EWallRunState::WallRunning)
    {
        DrawDebugLine(GetWorld(), ActorLocation, ActorLocation + (CurrentWallNormal * 200.0f), 
            FColor::Red, false, 0.0f, 0, 3.0f);
        DrawDebugLine(GetWorld(), ActorLocation, ActorLocation + (WallRunDirection * 300.0f), 
            FColor::Blue, false, 0.0f, 0, 3.0f);
    }
    
    // Draw state info without timer
    FString StateNames[] = {TEXT("None"), TEXT("Detecting"), TEXT("WallRunning"), TEXT("Falling"), TEXT("CoyoteTime")};
    FString StateName = (int32)CurrentState < 5 ? StateNames[(int32)CurrentState] : TEXT("Unknown");
    FString StateText = FString::Printf(TEXT("State: %s (%d), Side: %d, Speed: %.1f"), 
        *StateName, (int32)CurrentState, (int32)CurrentWallSide, CurrentWallRunSpeed);
    DrawDebugString(GetWorld(), ActorLocation + FVector(0, 0, 100), StateText, 
        nullptr, FColor::White, 0.0f);
}

void UWallRunComponent::LogWallRunInfo(const FString& Message) const
{
    if (bShowDebugLogs)
    {
        UE_LOG(LogTemp, Warning, TEXT("WallRun: %s"), *Message);
    }
}

// Wall verification functions
void UWallRunComponent::VerifyWallPresence()
{
    if (CurrentState != EWallRunState::WallRunning)
    {
        return; // Only verify during wall running
    }
    
    // Use existing wall detection to check if wall is still there
    FVector WallNormal;
    EWallSide WallSide;
    
    if (!DetectWall(WallNormal, WallSide))
    {
        // No wall detected - end wall run
        if (bShowDebugLogs)
        {
            LogWallRunInfo("Wall verification failed - no wall detected, ending wall run");
        }
        
        // Debug message removed - wall ended
        
        EndWallRun(false); // End without jump
    }
    else
    {
        // Wall still detected - update wall data in case wall angle changed
        CurrentWallNormal = WallNormal;
        CurrentWallSide = WallSide;
        WallRunDirection = CalculateWallRunDirection(WallNormal, WallSide);
        
        if (bShowDebugLogs)
        {
            LogWallRunInfo("Wall verification passed - wall still present");
        }
    }
}

void UWallRunComponent::StartWallVerificationTimer()
{
    if (!GetWorld())
    {
        return;
    }
    
    // Clear any existing timer
    StopWallVerificationTimer();
    
    // Start timer to check wall presence every second
    GetWorld()->GetTimerManager().SetTimer(
        WallVerificationTimer,
        this,
        &UWallRunComponent::VerifyWallPresence,
        1.0f, // Check every second
        true   // Repeating timer
    );
    
    if (bShowDebugLogs)
    {
        LogWallRunInfo("Started wall verification timer (1s intervals)");
    }
}

void UWallRunComponent::StopWallVerificationTimer()
{
    if (!GetWorld())
    {
        return;
    }
    
    // Clear the timer
    if (WallVerificationTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(WallVerificationTimer);
        WallVerificationTimer.Invalidate();
        
        if (bShowDebugLogs)
        {
            LogWallRunInfo("Stopped wall verification timer");
        }
    }
}

void UWallRunComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up wall verification timer
    StopWallVerificationTimer();
    
    // Clean up effects
    StopVisualEffects();
    
    // Restore movement if needed
    if (CurrentState == EWallRunState::WallRunning)
    {
        RestoreNormalMovement();
        RemoveWallRunGameplayTags();
    }
    
    Super::EndPlay(EndPlayReason);
}