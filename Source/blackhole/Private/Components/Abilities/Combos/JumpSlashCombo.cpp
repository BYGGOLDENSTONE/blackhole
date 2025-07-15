#include "Components/Abilities/Combos/JumpSlashCombo.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Systems/HitStopManager.h"
#include "Engine/DamageEvents.h"
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
    AimForgivenessRadius = 200.0f; // Increased for easier targeting
    
    // Default visual
    ComboTrailColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f); // Blue
    ShockwaveColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);
    
    // Override parent: Jump + Slash are both basic abilities, so combo should be basic too
    bIsBasicAbility = true;
    WPCost = 0.0f; // Basic abilities don't add WP corruption
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
        Start = Camera->GetComponentLocation();
        Forward = Camera->GetForwardVector();
    }
    
    // Find best target with aim forgiveness
    AActor* Target = FindBestTarget(Start, Forward, ComboRange);
    
    if (Target)
    {
        // Apply aerial damage using actor's TakeDamage (routes to WP)
        float FinalDamage = Damage * DamageMultiplier * GetDamageMultiplier();
        
        FVector ImpactDirection = (Target->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
        FPointDamageEvent DamageEvent(FinalDamage, FHitResult(), ImpactDirection, nullptr);
        Target->TakeDamage(FinalDamage, DamageEvent, nullptr, OwnerCharacter);
        
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
                // Apply shockwave damage to characters
                if (Cast<ACharacter>(HitActor))
                {
                    // Calculate distance-based damage falloff
                    float Distance = FVector::Dist(ShockwaveOrigin, HitActor->GetActorLocation());
                    float DamageFalloff = 1.0f - (Distance / ShockwaveRadius);
                    float FinalDamage = ShockwaveDamage * DamageFalloff * GetDamageMultiplier();
                    
                    // Use actor's TakeDamage (routes to WP)
                    FVector ImpactDirection = (HitActor->GetActorLocation() - ShockwaveOrigin).GetSafeNormal();
                    FPointDamageEvent DamageEvent(FinalDamage, FHitResult(), ImpactDirection, nullptr);
                    HitActor->TakeDamage(FinalDamage, DamageEvent, nullptr, OwnerCharacter);
                    
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

AActor* UJumpSlashCombo::FindBestTarget(const FVector& Start, const FVector& Forward, float SearchRange)
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
        // Check if it's a valid target (has WP to damage)
        if (Cast<ACharacter>(DirectTarget))
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
        
        // Check if it's a valid target (character that can be damaged)
        if (!Cast<ACharacter>(HitActor)) continue;
        
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
        FColor SearchColor = bPlayerOnGround ? FColor::Magenta : FColor::Blue;
        DrawDebugSphere(CachedWorld, Start, SphereRadius, 16, SearchColor, false, 0.5f);
        DrawDebugSphere(CachedWorld, End, SphereRadius, 16, SearchColor, false, 0.5f);
        DrawDebugLine(CachedWorld, Start, End, SearchColor, false, 0.5f, 0, 2.0f);
        
        // Highlight the selected target
        if (BestTarget)
        {
            FColor TargetColor = bPlayerOnGround ? FColor::Purple : FColor::Green;
            DrawDebugSphere(CachedWorld, BestTarget->GetActorLocation(), 50.0f, 12, TargetColor, false, 1.0f, 0, 3.0f);
        }
    }
    #endif
    
    return BestTarget;
}