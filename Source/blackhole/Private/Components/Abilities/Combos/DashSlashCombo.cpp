#include "Components/Abilities/Combos/DashSlashCombo.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Systems/HitStopManager.h"
#include "Camera/CameraComponent.h"
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
    
    FVector End = Start + (Forward * ComboRange);
    
    // Find target
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    
    if (CachedWorld->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
    {
        if (AActor* Target = HitResult.GetActor())
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