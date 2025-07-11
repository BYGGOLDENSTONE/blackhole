#include "Components/Abilities/Combos/DashSlashCombo.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Systems/HitStopManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Components/CapsuleComponent.h"
#include "Particles/ParticleSystemComponent.h"

UDashSlashCombo::UDashSlashCombo()
{
    // Set default values
    ComboWindowTime = 0.5f;
    TimeSlowScale = 0.1f; // Very slow for dramatic effect
    TimeSlowDuration = 0.25f;
    DamageMultiplier = 1.5f;
    ComboRange = 400.0f; // Extended range for dash
    
    // Phantom Strike specific
    TeleportDistance = 150.0f;
    BackstabDamageMultiplier = 2.0f;
    bAutoRotateToTarget = true;
    PhantomAfterImageDuration = 0.5f;
    AimForgivenessRadius = 180.0f; // Increased for easier targeting
    
    // Default visual
    ComboTrailColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f); // Cyan
}

void UDashSlashCombo::ExecuteCombo()
{
    if (!IsValid(OwnerCharacter) || !IsValid(CachedWorld)) return;
    
    // Get aiming direction
    FVector Start = OwnerCharacter->GetActorLocation();
    FVector Forward = OwnerCharacter->GetActorForwardVector();
    
    // Use camera for aiming if available
    if (UCameraComponent* Camera = OwnerCharacter->GetCameraComponent())
    {
        Start = Camera->GetComponentLocation();
        Forward = Camera->GetForwardVector();
    }
    
    // Find best target with aim forgiveness
    AActor* Target = FindBestTarget(Start, Forward, ComboRange);
    
    if (Target)
    {
        // Create phantom after-image before teleporting
        CreatePhantomAfterImage();
        
        // Teleport behind target
        if (TeleportBehindTarget(Target))
        {
            // Apply time slow for dramatic effect
            ApplyTimeSlow();
            
            // Play teleport effects
            if (TeleportParticle)
            {
                UGameplayStatics::SpawnEmitterAtLocation(CachedWorld, TeleportParticle, 
                    OwnerCharacter->GetActorLocation());
            }
            if (TeleportSound)
            {
                UGameplayStatics::PlaySoundAtLocation(CachedWorld, TeleportSound, 
                    OwnerCharacter->GetActorLocation());
            }
            
            // Apply backstab damage
            if (UIntegrityComponent* TargetIntegrity = Target->FindComponentByClass<UIntegrityComponent>())
            {
                float FinalDamage = Damage * DamageMultiplier * BackstabDamageMultiplier * GetDamageMultiplier();
                TargetIntegrity->TakeDamage(FinalDamage);
                
                // NOTE: Disabled hit stop to avoid conflicts with combo time slow
                // The combo time slow provides the dramatic effect we want
                /*
                if (UHitStopManager* HitStopMgr = CachedWorld->GetSubsystem<UHitStopManager>())
                {
                    ApplyHitStop(HitStopMgr, FinalDamage);
                }
                */
                
                // Visual feedback
                DrawComboVisuals(Start, Target->GetActorLocation());
                PlayComboFeedback(Target->GetActorLocation());
            }
        }
    }
    
    // Resource consumption is handled by the base Execute() that called this
}

bool UDashSlashCombo::TeleportBehindTarget(AActor* Target)
{
    if (!Target || !OwnerCharacter) return false;
    
    // Calculate teleport position
    FVector TargetLocation = Target->GetActorLocation();
    FVector TargetForward = Target->GetActorForwardVector();
    FVector TeleportLocation = TargetLocation - (TargetForward * TeleportDistance);
    
    // Check if teleport location is valid
    FHitResult GroundHit;
    FVector GroundCheckStart = TeleportLocation + FVector(0, 0, 100);
    FVector GroundCheckEnd = TeleportLocation - FVector(0, 0, 200);
    
    if (CachedWorld->LineTraceSingleByChannel(GroundHit, GroundCheckStart, GroundCheckEnd, 
        ECC_WorldStatic))
    {
        TeleportLocation.Z = GroundHit.Location.Z + OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    }
    
    // Perform teleport
    bool bTeleported = OwnerCharacter->SetActorLocation(TeleportLocation, false, nullptr, ETeleportType::TeleportPhysics);
    
    if (bTeleported && bAutoRotateToTarget)
    {
        // Face the target
        FRotator LookAtRotation = (TargetLocation - TeleportLocation).Rotation();
        LookAtRotation.Pitch = 0;
        LookAtRotation.Roll = 0;
        OwnerCharacter->SetActorRotation(LookAtRotation);
    }
    
    return bTeleported;
}

void UDashSlashCombo::CreatePhantomAfterImage()
{
    // TODO: Implement after-image effect
    // This could spawn a translucent copy of the character mesh that fades out
    
    #if WITH_EDITOR
    if (bShowDebugVisuals && OwnerCharacter)
    {
        // Debug visualization of after-image
        DrawDebugCapsule(CachedWorld, 
            OwnerCharacter->GetActorLocation(),
            OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
            OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius(),
            OwnerCharacter->GetActorQuat(),
            FColor(0, 200, 255, 100), // Semi-transparent cyan
            false,
            PhantomAfterImageDuration,
            0,
            1.0f);
    }
    #endif
}

AActor* UDashSlashCombo::FindBestTarget(const FVector& Start, const FVector& Forward, float SearchRange)
{
    if (!CachedWorld) return nullptr;
    
    // Check if player is on ground for different targeting behavior
    bool bPlayerOnGround = false;
    if (OwnerCharacter)
    {
        if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
        {
            bPlayerOnGround = Movement->IsMovingOnGround();
        }
    }
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    QueryParams.bTraceComplex = false;
    
    // First, try a direct line trace to see if we hit something in the crosshair
    FVector End = Start + (Forward * SearchRange);
    FHitResult DirectHit;
    bool bDirectHit = CachedWorld->LineTraceSingleByChannel(DirectHit, Start, End, ECC_Pawn, QueryParams);
    
    if (bDirectHit && DirectHit.GetActor())
    {
        AActor* DirectTarget = DirectHit.GetActor();
        if (DirectTarget->FindComponentByClass<UIntegrityComponent>())
        {
            // If we have a direct hit and player is on ground, prefer this target
            if (bPlayerOnGround)
            {
                #if WITH_EDITOR
                if (bShowDebugVisuals)
                {
                    DrawDebugSphere(CachedWorld, DirectTarget->GetActorLocation(), 60.0f, 12, FColor::Orange, false, 1.0f, 0, 4.0f);
                    DrawDebugLine(CachedWorld, Start, DirectTarget->GetActorLocation(), FColor::Orange, false, 1.0f, 0, 3.0f);
                }
                #endif
                return DirectTarget;
            }
        }
    }
    
    // Use sphere trace for aim forgiveness
    TArray<FHitResult> HitResults;
    float SphereRadius = AimForgivenessRadius;
    
    // Sphere sweep to find all potential targets
    CachedWorld->SweepMultiByChannel(
        HitResults,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(SphereRadius),
        QueryParams
    );
    
    // Find the best target
    AActor* BestTarget = nullptr;
    float BestScore = -1.0f;
    float ClosestDistance = FLT_MAX;
    
    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor) continue;
        
        // Check if it has integrity component (can be damaged)
        if (!HitActor->FindComponentByClass<UIntegrityComponent>()) continue;
        
        FVector ToTarget = HitActor->GetActorLocation() - Start;
        float Distance = ToTarget.Size();
        ToTarget.Normalize();
        
        // Only consider targets in front of us
        float ForwardDot = FVector::DotProduct(Forward, ToTarget);
        if (ForwardDot <= 0) continue;
        
        // When on ground, heavily prioritize closest enemy
        if (bPlayerOnGround)
        {
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                BestTarget = HitActor;
            }
            continue;
        }
        
        // Air targeting - use the original scoring system
        // Separate horizontal and vertical scoring
        FVector ForwardHorizontal = Forward;
        ForwardHorizontal.Z = 0;
        ForwardHorizontal.Normalize();
        
        FVector ToTargetHorizontal = ToTarget;
        ToTargetHorizontal.Z = 0;
        if (ToTargetHorizontal.Size() > 0.01f) // Avoid division by zero
        {
            ToTargetHorizontal.Normalize();
        }
        
        // Horizontal angle score (more important)
        float HorizontalDot = FVector::DotProduct(ForwardHorizontal, ToTargetHorizontal);
        float HorizontalScore = FMath::Max(0.0f, HorizontalDot);
        
        // Vertical offset score (very forgiving)
        float VerticalOffset = FMath::Abs(ToTarget.Z);
        float VerticalScore = 1.0f - (VerticalOffset * 0.3f); // Very lenient with vertical
        VerticalScore = FMath::Clamp(VerticalScore, 0.0f, 1.0f);
        
        // Distance score
        float DistanceScore = 1.0f - (Distance / SearchRange);
        DistanceScore = FMath::Max(0.0f, DistanceScore);
        
        // Combined score - prioritize horizontal alignment
        float Score = (HorizontalScore * 0.5f) + (VerticalScore * 0.3f) + (DistanceScore * 0.2f);
        
        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = HitActor;
        }
    }
    
    // Visual debug
    #if WITH_EDITOR
    if (bShowDebugVisuals)
    {
        // Show the search area with different colors for ground vs air
        FColor SearchColor = bPlayerOnGround ? FColor::Red : FColor::Yellow;
        DrawDebugSphere(CachedWorld, Start, SphereRadius, 16, SearchColor, false, 0.5f);
        DrawDebugSphere(CachedWorld, End, SphereRadius, 16, SearchColor, false, 0.5f);
        DrawDebugLine(CachedWorld, Start, End, SearchColor, false, 0.5f, 0, 2.0f);
        
        // Highlight the selected target
        if (BestTarget)
        {
            FColor TargetColor = bPlayerOnGround ? FColor::Cyan : FColor::Green;
            DrawDebugSphere(CachedWorld, BestTarget->GetActorLocation(), 50.0f, 12, TargetColor, false, 1.0f, 0, 3.0f);
        }
    }
    #endif
    
    return BestTarget;
}