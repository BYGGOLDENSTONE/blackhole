#include "Components/Movement/WallRunComponent.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"

UWallRunComponent::UWallRunComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
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
    
    LogWallRunInfo("WallRunComponent: Initialized successfully");
}

void UWallRunComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!OwnerCharacter || !MovementComponent)
    {
        return;
    }
    
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
            if (HasForwardInput() && HasJumpInput() && CanStartWallRun())
            {
                if (TryStartWallRun())
                {
                    ChangeState(EWallRunState::WallRunning);
                }
                else
                {
                    // Start coyote time if we're airborne near a wall
                    if (!MovementComponent->IsMovingOnGround())
                    {
                        ChangeState(EWallRunState::CoyoteTime);
                        CoyoteTimer = 0.0f;
                    }
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
            UpdateWallRunMovement(DeltaTime);
            UpdateWallDetection(DeltaTime);
            
            // Update timer and check for timeout
            WallRunTimer += DeltaTime;
            OnWallRunTimerUpdate.Broadcast(GetWallRunTimeRemaining());
            
            if (WallRunTimer >= Settings.MaxWallRunDuration)
            {
                LogWallRunInfo("Wall run timer expired");
                EndWallRun(false);
            }
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
    
    // Try to start wall run during coyote time
    if (HasForwardInput() && TryStartWallRun())
    {
        ChangeState(EWallRunState::WallRunning);
        LogWallRunInfo("Wall run started during coyote time!");
        return;
    }
    
    // End coyote time
    if (CoyoteTimer >= Settings.CoyoteTimeDuration)
    {
        ChangeState(EWallRunState::None);
        LogWallRunInfo("Coyote time expired");
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
            // Lost wall contact - start corner tolerance
            ChangeState(EWallRunState::Falling);
            CornerToleranceTimer = 0.0f;
            LogWallRunInfo("Lost wall contact - starting corner tolerance");
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
        LogWallRunInfo("Found new wall during corner tolerance - continuing wall run");
        return;
    }
    
    // End corner tolerance
    if (CornerToleranceTimer >= Settings.CornerToleranceDuration)
    {
        EndWallRun(false);
        LogWallRunInfo("Corner tolerance expired - ending wall run");
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
    
    if (WallAngle < Settings.MinWallAngle || WallAngle > Settings.MaxWallAngle)
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
    
    // Check wall jump cooldown
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastWallJumpTime < Settings.WallJumpCooldown)
    {
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
        return false;
    }
    
    return true;
}

bool UWallRunComponent::TryStartWallRun()
{
    FVector WallNormal;
    EWallSide WallSide;
    
    if (!DetectWall(WallNormal, WallSide))
    {
        return false;
    }
    
    StartWallRunInternal(WallNormal, WallSide);
    return true;
}

void UWallRunComponent::StartWallRunInternal(const FVector& WallNormal, EWallSide WallSide)
{
    // Store wall data
    CurrentWallNormal = WallNormal;
    CurrentWallSide = WallSide;
    LastValidWallNormal = WallNormal;
    WallRunDirection = CalculateWallRunDirection(WallNormal, WallSide);
    
    // Reset timer
    WallRunTimer = 0.0f;
    
    // Calculate initial speed with momentum preservation
    float CurrentSpeed = MovementComponent->Velocity.Size2D();
    CurrentWallRunSpeed = FMath::Max(Settings.WallRunSpeed, CurrentSpeed * Settings.SpeedBoostMultiplier);
    
    // Apply wall run physics
    ApplyWallRunPhysics();
    
    // Apply gameplay tags for ability restrictions
    ApplyWallRunGameplayTags();
    
    // Start visual and audio effects
    StartVisualEffects();
    
    LogWallRunInfo(FString::Printf(TEXT("Started wall run on %s side with speed %.1f"), 
        WallSide == EWallSide::Right ? TEXT("RIGHT") : TEXT("LEFT"), CurrentWallRunSpeed));
    
    OnWallRunStateChanged.Broadcast(EWallRunState::WallRunning);
}

void UWallRunComponent::ApplyWallRunPhysics()
{
    if (!MovementComponent)
    {
        return;
    }
    
    // Disable gravity
    MovementComponent->GravityScale = 0.0f;
    
    // Set movement mode to flying for better control
    MovementComponent->SetMovementMode(MOVE_Flying);
    
    // Apply initial wall run velocity
    ApplyWallRunMovement(0.0f);
}

void UWallRunComponent::ApplyWallRunMovement(float DeltaTime)
{
    if (!MovementComponent || CurrentState != EWallRunState::WallRunning)
    {
        return;
    }
    
    // Apply speed decay over time
    if (DeltaTime > 0.0f)
    {
        CurrentWallRunSpeed = FMath::Max(Settings.WallRunSpeed * 0.5f, 
            CurrentWallRunSpeed - (Settings.SpeedDecayRate * DeltaTime * 100.0f));
    }
    
    // Calculate wall run velocity
    FVector WallRunVelocity = WallRunDirection * CurrentWallRunSpeed;
    
    // Add slight upward velocity to counteract any downward drift
    WallRunVelocity.Z = FMath::Max(0.0f, WallRunVelocity.Z + 50.0f);
    
    // Apply velocity
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
        LogWallRunInfo(FString::Printf(TEXT("Wall jump with velocity: %s"), *LaunchVelocity.ToString()));
    }
    
    // Remove gameplay tags
    RemoveWallRunGameplayTags();
    
    // Stop visual and audio effects
    StopVisualEffects();
    
    // Reset state
    ChangeState(EWallRunState::None);
    CurrentWallSide = EWallSide::None;
    CurrentWallNormal = FVector::ZeroVector;
    WallRunDirection = FVector::ZeroVector;
    WallRunTimer = 0.0f;
    
    LogWallRunInfo(FString::Printf(TEXT("Ended wall run (Jump Cancel: %s)"), 
        bJumpCancel ? TEXT("YES") : TEXT("NO")));
    
    OnWallRunStateChanged.Broadcast(EWallRunState::None);
}

void UWallRunComponent::RestoreNormalMovement()
{
    if (!MovementComponent)
    {
        return;
    }
    
    // Restore gravity
    MovementComponent->GravityScale = OriginalGravityScale;
    
    // Restore movement mode
    MovementComponent->SetMovementMode(MOVE_Walking);
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
        // Launch away from wall and forward
        FVector AwayFromWall = -CurrentWallNormal * Settings.LaunchImpulseStrength * 0.6f;
        FVector ForwardLaunch = WallRunDirection * Settings.LaunchImpulseStrength * 0.8f;
        FVector UpwardLaunch = FVector::UpVector * Settings.LaunchImpulseStrength;
        
        LaunchVelocity = AwayFromWall + ForwardLaunch + UpwardLaunch;
    }
    else
    {
        // Just fall normally with some forward momentum
        LaunchVelocity = WallRunDirection * (CurrentWallRunSpeed * 0.5f);
    }
    
    return LaunchVelocity;
}

void UWallRunComponent::OnJumpPressed()
{
    if (CurrentState == EWallRunState::WallRunning)
    {
        LogWallRunInfo("Jump pressed during wall run - canceling");
        EndWallRun(true);
    }
}

bool UWallRunComponent::CanUseAbilityDuringWallRun(const UAbilityComponent* Ability) const
{
    if (!Ability || CurrentState != EWallRunState::WallRunning)
    {
        return true; // Allow all abilities when not wall running
    }
    
    // Special case: Always allow jump ability during wall run
    // (Jump is handled separately through OnJumpPressed)
    FString AbilityName = Ability->GetName();
    if (AbilityName.Contains(TEXT("Jump")))
    {
        return true;
    }
    
    // Block all basic abilities except jump
    if (Ability->IsBasicAbility())
    {
        return false;
    }
    
    // Allow advanced abilities during wall run
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
    LogWallRunInfo("Wall run state applied - abilities restricted");
}

void UWallRunComponent::RemoveWallRunGameplayTags()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Note: This project uses a custom ability system, not Unreal's GAS
    // The wall run restrictions are handled through CanUseAbilityDuringWallRun() instead
    LogWallRunInfo("Wall run state removed - abilities unrestricted");
}

float UWallRunComponent::GetWallRunTimeRemaining() const
{
    if (CurrentState != EWallRunState::WallRunning)
    {
        return 0.0f;
    }
    
    return FMath::Max(0.0f, Settings.MaxWallRunDuration - WallRunTimer);
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
    
    LogWallRunInfo(FString::Printf(TEXT("State changed from %d to %d"), 
        (int32)OldState, (int32)NewState));
    
    OnWallRunStateChanged.Broadcast(NewState);
}

bool UWallRunComponent::HasForwardInput() const
{
    if (!OwnerCharacter)
    {
        return false;
    }
    
    // Check if player is providing forward input
    FVector InputVector = OwnerCharacter->GetLastMovementInputVector();
    FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
    
    float ForwardInput = FVector::DotProduct(InputVector, ForwardVector);
    return ForwardInput > 0.1f;
}

bool UWallRunComponent::HasJumpInput() const
{
    if (!OwnerCharacter)
    {
        return false;
    }
    
    // This would need to be connected to the input system
    // For now, we'll assume it's checked externally via OnJumpPressed()
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
    
    // Draw state info
    FString StateText = FString::Printf(TEXT("State: %d, Side: %d, Speed: %.1f, Timer: %.1f"), 
        (int32)CurrentState, (int32)CurrentWallSide, CurrentWallRunSpeed, WallRunTimer);
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

void UWallRunComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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