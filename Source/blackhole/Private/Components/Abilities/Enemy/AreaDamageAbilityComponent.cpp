// AreaDamageAbilityComponent.cpp
#include "Components/Abilities/Enemy/AreaDamageAbilityComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"

UAreaDamageAbilityComponent::UAreaDamageAbilityComponent()
{
    bIsBasicAbility = false;
    WPCost = 0.0f; // Enemies don't use WP
    Cooldown = 5.0f; // Default cooldown for area damage
    Range = 800.0f; // Default range
}

void UAreaDamageAbilityComponent::Execute()
{
    if (!CanExecute())
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // Store origin and direction for later use
    DamageOrigin = Owner->GetActorLocation();
    DamageDirection = Owner->GetActorForwardVector();

    // Play pre-damage animation if configured
    if (PreDamageMontage && PreDamageDelay > 0.0f)
    {
        if (ACharacter* CharOwner = Cast<ACharacter>(Owner))
        {
            CharOwner->PlayAnimMontage(PreDamageMontage);
        }

        // Delay the actual damage
        Owner->GetWorld()->GetTimerManager().SetTimer(
            PreDamageTimerHandle,
            this,
            &UAreaDamageAbilityComponent::PerformAreaDamage,
            PreDamageDelay,
            false
        );
    }
    else
    {
        // No delay, perform damage immediately
        PerformAreaDamage();
    }

    // Start cooldown
    StartCooldown();
}

void UAreaDamageAbilityComponent::ExecuteUltimate()
{
    // Ultimate version: Double damage and radius
    float OriginalDamage = BaseDamage;
    float OriginalRadius = DamageRadius;
    
    BaseDamage *= 2.0f;
    DamageRadius *= 1.5f;
    
    Execute();
    
    // Restore original values
    BaseDamage = OriginalDamage;
    DamageRadius = OriginalRadius;
}

void UAreaDamageAbilityComponent::PerformAreaDamage()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // Clear old hit tracking entries
    float CurrentTime = GetWorld()->GetTimeSeconds();
    TArray<AActor*> ActorsToRemove;
    for (auto& Pair : RecentlyHitActors)
    {
        if (CurrentTime - Pair.Value > HitPreventionDuration)
        {
            ActorsToRemove.Add(Pair.Key);
        }
    }
    for (AActor* Actor : ActorsToRemove)
    {
        RecentlyHitActors.Remove(Actor);
    }

    // Get all actors in the damage pattern
    TArray<AActor*> AffectedActors = GetActorsInPattern();

    // Visual effects at origin
    PlayVisualEffects();

    // Apply damage to each actor
    for (AActor* Target : AffectedActors)
    {
        if (!Target || Target == Owner && !bDamageSelf)
        {
            continue;
        }
        
        // Skip if recently hit
        if (HasRecentlyHitActor(Target))
        {
            continue;
        }

        // Check targeting rules
        bool bShouldDamage = false;
        if (Target->IsA<ABlackholePlayerCharacter>() && bDamagePlayers)
        {
            bShouldDamage = true;
        }
        else if (Target->IsA<ABaseEnemy>() && bDamageEnemies)
        {
            bShouldDamage = true;
        }

        if (!bShouldDamage)
        {
            continue;
        }

        // Calculate distance-based damage
        float Distance = FVector::Dist(DamageOrigin, Target->GetActorLocation());
        float DamageFalloff = bUseDamageFalloff ? CalculateDamageFalloff(Distance) : 1.0f;
        float FinalDamage = BaseDamage * DamageFalloff;

        // Apply damage
        ApplyDamageToActor(Target, FinalDamage);

        // Apply knockback if enabled
        if (bApplyKnockback)
        {
            ApplyKnockbackToActor(Target);
        }

        // Apply post-damage effects
        ApplyPostEffectsToActor(Target);
        
        // Track this hit
        TrackHitActor(Target);
    }

    // Apply self-stagger if this is a tank-like enemy
    if (bApplyStagger && Owner->IsA<ABaseEnemy>())
    {
        if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(Owner))
        {
            Enemy->ApplyStagger(StaggerDuration);
        }
    }
}

TArray<AActor*> UAreaDamageAbilityComponent::GetActorsInPattern()
{
    TArray<AActor*> ResultActors;
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->GetWorld())
    {
        return ResultActors;
    }

    UWorld* World = Owner->GetWorld();

    switch (DamagePattern)
    {
        case EAreaDamagePattern::Circular:
        {
            // Standard circular area
            TArray<FOverlapResult> OverlapResults;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(Owner);

            World->OverlapMultiByChannel(
                OverlapResults,
                DamageOrigin,
                FQuat::Identity,
                ECC_Pawn,
                FCollisionShape::MakeSphere(DamageRadius),
                QueryParams
            );

            for (const FOverlapResult& Result : OverlapResults)
            {
                if (Result.GetActor())
                {
                    ResultActors.Add(Result.GetActor());
                }
            }
            break;
        }

        case EAreaDamagePattern::Cone:
        {
            // Get all actors in a sphere first, then filter by cone
            TArray<FOverlapResult> OverlapResults;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(Owner);

            World->OverlapMultiByChannel(
                OverlapResults,
                DamageOrigin,
                FQuat::Identity,
                ECC_Pawn,
                FCollisionShape::MakeSphere(DamageRadius),
                QueryParams
            );

            for (const FOverlapResult& Result : OverlapResults)
            {
                if (Result.GetActor() && IsActorInCone(Result.GetActor(), DamageOrigin, DamageDirection))
                {
                    ResultActors.Add(Result.GetActor());
                }
            }
            break;
        }

        case EAreaDamagePattern::Line:
        {
            // Line pattern damage
            TArray<FOverlapResult> OverlapResults;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(Owner);

            // Create a box for line detection
            FVector LineEnd = DamageOrigin + (DamageDirection * DamageRadius);
            FVector BoxExtent(LineWidth / 2.0f, LineWidth / 2.0f, 100.0f);

            World->OverlapMultiByChannel(
                OverlapResults,
                (DamageOrigin + LineEnd) / 2.0f,
                FQuat::Identity,
                ECC_Pawn,
                FCollisionShape::MakeBox(BoxExtent),
                QueryParams
            );

            for (const FOverlapResult& Result : OverlapResults)
            {
                if (Result.GetActor() && IsActorInLine(Result.GetActor(), DamageOrigin, DamageDirection))
                {
                    ResultActors.Add(Result.GetActor());
                }
            }
            break;
        }

        case EAreaDamagePattern::Cross:
        {
            // Cross pattern - combination of two perpendicular lines
            TArray<FOverlapResult> OverlapResults;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(Owner);

            World->OverlapMultiByChannel(
                OverlapResults,
                DamageOrigin,
                FQuat::Identity,
                ECC_Pawn,
                FCollisionShape::MakeSphere(DamageRadius),
                QueryParams
            );

            for (const FOverlapResult& Result : OverlapResults)
            {
                if (Result.GetActor() && IsActorInCross(Result.GetActor(), DamageOrigin))
                {
                    ResultActors.Add(Result.GetActor());
                }
            }
            break;
        }
    }

    return ResultActors;
}

bool UAreaDamageAbilityComponent::IsActorInCone(AActor* Actor, const FVector& ConeOrigin, const FVector& ConeDirection) const
{
    FVector ToTarget = (Actor->GetActorLocation() - ConeOrigin).GetSafeNormal();
    float DotProduct = FVector::DotProduct(ConeDirection, ToTarget);
    float Angle = FMath::Acos(DotProduct) * (180.0f / PI);
    return Angle <= (ConeAngle / 2.0f);
}

bool UAreaDamageAbilityComponent::IsActorInLine(AActor* Actor, const FVector& LineStart, const FVector& LineDirection) const
{
    FVector ToTarget = Actor->GetActorLocation() - LineStart;
    FVector LineNormal = LineDirection.GetSafeNormal();
    
    // Project target onto line
    float ProjectionLength = FVector::DotProduct(ToTarget, LineNormal);
    
    // Check if within line length
    if (ProjectionLength < 0.0f || ProjectionLength > DamageRadius)
    {
        return false;
    }
    
    // Check perpendicular distance
    FVector ClosestPointOnLine = LineStart + LineNormal * ProjectionLength;
    float PerpendicularDistance = FVector::Dist(Actor->GetActorLocation(), ClosestPointOnLine);
    
    return PerpendicularDistance <= (LineWidth / 2.0f);
}

bool UAreaDamageAbilityComponent::IsActorInCross(AActor* Actor, const FVector& CrossCenter) const
{
    FVector ToTarget = Actor->GetActorLocation() - CrossCenter;
    
    // Check if in horizontal or vertical line of the cross
    float AbsX = FMath::Abs(ToTarget.X);
    float AbsY = FMath::Abs(ToTarget.Y);
    
    bool bInHorizontal = AbsX <= DamageRadius && AbsY <= (LineWidth / 2.0f);
    bool bInVertical = AbsY <= DamageRadius && AbsX <= (LineWidth / 2.0f);
    
    return bInHorizontal || bInVertical;
}

float UAreaDamageAbilityComponent::CalculateDamageFalloff(float Distance) const
{
    if (!bUseDamageFalloff || Distance <= 0.0f)
    {
        return 1.0f;
    }
    
    float NormalizedDistance = FMath::Clamp(Distance / DamageRadius, 0.0f, 1.0f);
    return FMath::Lerp(1.0f, MinDamagePercent, NormalizedDistance);
}

void UAreaDamageAbilityComponent::ApplyDamageToActor(AActor* Target, float FinalDamage)
{
    if (!Target)
    {
        return;
    }

    // Create damage event with custom damage type if specified
    FPointDamageEvent DamageEvent(FinalDamage, FHitResult(), DamageOrigin, nullptr);
    DamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass.Get() : UDamageType::StaticClass();
    
    // Apply damage - this will route through the game's damage system
    // For players, this will damage WP if bDamagePlayerWP is true
    Target->TakeDamage(FinalDamage, DamageEvent, nullptr, GetOwner());
}

void UAreaDamageAbilityComponent::ApplyKnockbackToActor(AActor* Target)
{
    ACharacter* TargetCharacter = Cast<ACharacter>(Target);
    if (!TargetCharacter || !TargetCharacter->GetCharacterMovement())
    {
        return;
    }

    // Calculate knockback direction
    FVector KnockbackDirection = (Target->GetActorLocation() - DamageOrigin).GetSafeNormal();
    
    // Add upward component
    KnockbackDirection.Z = KnockbackUpRatio;
    KnockbackDirection.Normalize();
    
    // Apply knockback
    FVector KnockbackVelocity = KnockbackDirection * KnockbackForce;
    TargetCharacter->GetCharacterMovement()->Launch(KnockbackVelocity);
}

void UAreaDamageAbilityComponent::ApplyPostEffectsToActor(AActor* Target)
{
    ACharacter* TargetCharacter = Cast<ACharacter>(Target);
    if (!TargetCharacter || !TargetCharacter->GetCharacterMovement())
    {
        return;
    }

    // Apply slow effect
    if (bApplySlow)
    {
        float OriginalSpeed = TargetCharacter->GetCharacterMovement()->MaxWalkSpeed;
        float SlowedSpeed = OriginalSpeed * (1.0f - SlowPercent);
        
        TargetCharacter->GetCharacterMovement()->MaxWalkSpeed = SlowedSpeed;
        
        // Set timer to restore speed
        FTimerHandle SlowTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            SlowTimerHandle,
            [TargetCharacter, OriginalSpeed]()
            {
                if (IsValid(TargetCharacter) && TargetCharacter->GetCharacterMovement())
                {
                    TargetCharacter->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeed;
                }
            },
            SlowDuration,
            false
        );
    }

    // Note: Stagger for self is handled in PerformAreaDamage()
}

void UAreaDamageAbilityComponent::PlayVisualEffects()
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->GetWorld())
    {
        return;
    }

    // Spawn particle effect
    if (VisualEffects.ImpactParticle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            Owner->GetWorld(),
            VisualEffects.ImpactParticle,
            DamageOrigin,
            DamageDirection.Rotation()
        );
    }

    // Play sound
    if (VisualEffects.ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            Owner->GetWorld(),
            VisualEffects.ImpactSound,
            DamageOrigin
        );
    }

    // Camera shake
    if (VisualEffects.CameraShake)
    {
        UGameplayStatics::PlayWorldCameraShake(
            Owner->GetWorld(),
            VisualEffects.CameraShake,
            DamageOrigin,
            VisualEffects.CameraShakeRadius,
            1.0f
        );
    }

    // Debug visualization
    if (bShowDebugVisualization)
    {
        switch (DamagePattern)
        {
            case EAreaDamagePattern::Circular:
                DrawDebugSphere(GetWorld(), DamageOrigin, DamageRadius, 32, FColor::Red, false, 2.0f);
                break;
            
            case EAreaDamagePattern::Cone:
            {
                // Draw cone visualization
                FVector LeftEdge = DamageDirection.RotateAngleAxis(-ConeAngle/2, FVector::UpVector) * DamageRadius;
                FVector RightEdge = DamageDirection.RotateAngleAxis(ConeAngle/2, FVector::UpVector) * DamageRadius;
                
                DrawDebugLine(GetWorld(), DamageOrigin, DamageOrigin + LeftEdge, FColor::Red, false, 2.0f, 0, 3.0f);
                DrawDebugLine(GetWorld(), DamageOrigin, DamageOrigin + RightEdge, FColor::Red, false, 2.0f, 0, 3.0f);
                DrawDebugLine(GetWorld(), DamageOrigin + LeftEdge, DamageOrigin + RightEdge, FColor::Red, false, 2.0f, 0, 3.0f);
                break;
            }
            
            case EAreaDamagePattern::Line:
            {
                FVector LineEnd = DamageOrigin + (DamageDirection * DamageRadius);
                DrawDebugBox(GetWorld(), (DamageOrigin + LineEnd) / 2.0f, FVector(DamageRadius/2, LineWidth/2, 100), FColor::Red, false, 2.0f);
                break;
            }
            
            case EAreaDamagePattern::Cross:
            {
                DrawDebugLine(GetWorld(), DamageOrigin - FVector(DamageRadius, 0, 0), DamageOrigin + FVector(DamageRadius, 0, 0), FColor::Red, false, 2.0f, 0, LineWidth);
                DrawDebugLine(GetWorld(), DamageOrigin - FVector(0, DamageRadius, 0), DamageOrigin + FVector(0, DamageRadius, 0), FColor::Red, false, 2.0f, 0, LineWidth);
                break;
            }
        }
    }
}

bool UAreaDamageAbilityComponent::HasRecentlyHitActor(AActor* Actor) const
{
    return RecentlyHitActors.Contains(Actor);
}

void UAreaDamageAbilityComponent::TrackHitActor(AActor* Actor)
{
    if (!Actor || !GetWorld())
    {
        return;
    }
    
    RecentlyHitActors.Add(Actor, GetWorld()->GetTimeSeconds());
}