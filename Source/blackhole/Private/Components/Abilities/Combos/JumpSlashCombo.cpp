#include "Components/Abilities/Combos/JumpSlashCombo.h"
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
#include "TimerManager.h"
#include "Engine/OverlapResult.h"

UJumpSlashCombo::UJumpSlashCombo()
{
    // Set default values
    ComboWindowTime = 0.3f; // Tight window while airborne
    TimeSlowScale = 0.2f;
    TimeSlowDuration = 0.2f;
    DamageMultiplier = 1.25f;
    ComboRange = 250.0f;
    
    // Aerial Rave specific
    ShockwaveRadius = 300.0f;
    ShockwaveDamage = 50.0f;
    ShockwaveDelay = 0.3f;
    bRequireAirborne = true;
    DownwardForce = 1000.0f;
    
    // Default visual
    ComboTrailColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f); // Blue
    ShockwaveColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);
}

void UJumpSlashCombo::ExecuteCombo()
{
    if (!IsValid(OwnerCharacter) || !IsValid(CachedWorld)) return;
    
    // Check if airborne requirement is met
    if (bRequireAirborne && !IsCharacterAirborne())
    {
        UE_LOG(LogTemp, Warning, TEXT("JumpSlashCombo: Character must be airborne"));
        return;
    }
    
    // Apply downward slash
    ApplyDownwardSlash();
    
    // Apply time slow
    ApplyTimeSlow();
    
    // Schedule shockwave creation
    if (ShockwaveDelay > 0.0f)
    {
        CachedWorld->GetTimerManager().SetTimer(ShockwaveTimerHandle,
            this, &UJumpSlashCombo::CreateShockwave, ShockwaveDelay, false);
    }
    else
    {
        CreateShockwave();
    }
    
    // Resource consumption is handled by the base Execute() that called this
}

void UJumpSlashCombo::ApplyDownwardSlash()
{
    if (!IsValid(OwnerCharacter) || !IsValid(CachedWorld)) return;
    
    // Get aiming direction
    FVector Start = OwnerCharacter->GetActorLocation();
    FVector Forward = OwnerCharacter->GetActorForwardVector();
    
    // Use camera for aiming if available
    if (UCameraComponent* Camera = OwnerCharacter->GetCameraComponent())
    {
        Forward = Camera->GetForwardVector();
    }
    
    FVector End = Start + (Forward * ComboRange);
    
    // Perform slash hit detection
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    
    if (CachedWorld->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
    {
        if (AActor* Target = HitResult.GetActor())
        {
            if (UIntegrityComponent* TargetIntegrity = Target->FindComponentByClass<UIntegrityComponent>())
            {
                // Apply aerial damage
                float FinalDamage = Damage * DamageMultiplier * GetDamageMultiplier();
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
    
    // Apply downward force
    if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
    {
        FVector CurrentVelocity = Movement->Velocity;
        CurrentVelocity.Z = -DownwardForce;
        Movement->Velocity = CurrentVelocity;
    }
}

void UJumpSlashCombo::CreateShockwave()
{
    if (!IsValid(OwnerCharacter) || !IsValid(CachedWorld)) return;
    
    FVector ShockwaveOrigin = OwnerCharacter->GetActorLocation();
    
    // Find all enemies in shockwave radius
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ShockwaveRadius);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    
    if (CachedWorld->OverlapMultiByChannel(OverlapResults, ShockwaveOrigin, FQuat::Identity, 
        ECC_Pawn, CollisionShape, QueryParams))
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            if (AActor* HitActor = Result.GetActor())
            {
                if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
                {
                    // Calculate distance-based damage falloff
                    float Distance = FVector::Dist(ShockwaveOrigin, HitActor->GetActorLocation());
                    float DamageFalloff = 1.0f - (Distance / ShockwaveRadius);
                    float FinalDamage = ShockwaveDamage * DamageFalloff * GetDamageMultiplier();
                    
                    TargetIntegrity->TakeDamage(FinalDamage);
                    
                    // NOTE: Disabled hit stop to avoid conflicts with combo time slow
                    /*
                    if (UHitStopManager* HitStopMgr = CachedWorld->GetSubsystem<UHitStopManager>())
                    {
                        HitStopMgr->RequestLightHitStop();
                    }
                    */
                }
            }
        }
    }
    
    // Visual effects
    if (ShockwaveParticle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(CachedWorld, ShockwaveParticle, ShockwaveOrigin);
    }
    
    if (ShockwaveSound)
    {
        UGameplayStatics::PlaySoundAtLocation(CachedWorld, ShockwaveSound, ShockwaveOrigin);
    }
    
    // Debug visualization
    #if WITH_EDITOR
    if (bShowDebugVisuals)
    {
        // Draw expanding rings
        for (int32 i = 1; i <= 3; i++)
        {
            float RingRadius = (ShockwaveRadius / 3.0f) * i;
            DrawDebugSphere(CachedWorld, ShockwaveOrigin, RingRadius, 
                24, ShockwaveColor.ToFColor(true), false, 1.0f);
        }
    }
    #endif
}

bool UJumpSlashCombo::IsCharacterAirborne() const
{
    if (!IsValid(OwnerCharacter)) return false;
    
    if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
    {
        return Movement->IsFalling();
    }
    
    return false;
}