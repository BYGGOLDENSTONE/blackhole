#include "Components/Abilities/Player/Hacker/GravityPullAbilityComponent.h"
#include "Components/Interaction/HackableComponent.h"
#include "Systems/ResourceManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "Actors/PsiDisruptor.h"
#include "Enemy/TankEnemy.h"

UGravityPullAbilityComponent::UGravityPullAbilityComponent()
{
    // Set default values
    WPCost = 15.0f; // New dual resource system
    // HeatCost removed - heat system no longer exists
    Cooldown = 3.0f;       // 3 second cooldown
    Range = 2000.0f;       // 20 meter range
    
    // Default force values (will be applied as impulse)
    LaunchForceMultiplier = 1.0f;
    BaseLaunchForce = 2500.0f;  // Good force for enemy pull
    
    // Ensure this is NOT a basic ability
    bIsBasicAbility = false;
}

void UGravityPullAbilityComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UGravityPullAbilityComponent::Execute()
{
    if (!CanExecute()) return;

    // ALWAYS call base class first - it will handle ultimate detection and execution
    Super::Execute();

    // If we executed as ultimate, we're done
    if (IsInUltimateMode())
    {
        UE_LOG(LogTemp, Warning, TEXT("GravityPull: Ultimate version was executed"));
        return;
    }

    // Normal (non-ultimate) execution continues here
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Get camera location and direction for trace
    FVector CameraLocation;
    FVector CameraDirection;
    GetCameraLocationAndDirection(CameraLocation, CameraDirection);

    // Perform line trace to find target
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Owner);
    QueryParams.bTraceComplex = false;

    FVector TraceEnd = CameraLocation + (CameraDirection * Range);
    
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        CameraLocation,
        TraceEnd,
        ECC_Pawn,
        QueryParams
    );

    // Debug visualization
    #if WITH_EDITOR
    DrawDebugLine(GetWorld(), CameraLocation, TraceEnd, FColor::Cyan, false, 1.0f, 0, 2.0f);
    #endif

    if (!bHit || !HitResult.GetActor())
    {
        UE_LOG(LogTemp, Warning, TEXT("GravityPull: No target hit by trace"));
        return;
    }

    AActor* Target = HitResult.GetActor();

    // Check if it's an enemy
    ACharacter* TargetCharacter = Cast<ACharacter>(Target);
    if (!TargetCharacter || !Target->ActorHasTag("Enemy"))
    {
        UE_LOG(LogTemp, Warning, TEXT("GravityPull: Target is not an enemy"));
        return;
    }

    // Check if it's a Tank enemy - they are immune
    if (Target->IsA<ATankEnemy>())
    {
        UE_LOG(LogTemp, Warning, TEXT("GravityPull: Tank enemies are immune to Gravity Pull"));
        return;
    }

    // Calculate current distance
    FVector PlayerLocation = Owner->GetActorLocation();
    float CurrentDistance = FVector::Dist(PlayerLocation, Target->GetActorLocation());
    
    // Calculate target position using configurable ratio
    float TargetDistance = CurrentDistance * PullDistanceRatio;
    
    // Ensure minimum distance
    if (TargetDistance < MinimumPullDistance)
    {
        TargetDistance = MinimumPullDistance;
    }

    // Calculate the target position
    FVector DirectionToPlayer = (PlayerLocation - Target->GetActorLocation()).GetSafeNormal();
    FVector TargetPosition = PlayerLocation - (DirectionToPlayer * TargetDistance);

    // Calculate pull force needed to reach target position
    FVector PullDirection = (TargetPosition - Target->GetActorLocation()).GetSafeNormal();
    float PullForce = BaseLaunchForce * LaunchForceMultiplier;
    
    // Add upward bounce component
    FVector FinalPullVelocity = PullDirection * PullForce;
    FinalPullVelocity.Z += BounceUpwardForce;

    // Apply the pull force with bounce
    TargetCharacter->LaunchCharacter(FinalPullVelocity, true, true);

    // Visual feedback
    #if WITH_EDITOR
    DrawDebugSphere(GetWorld(), TargetPosition, 50.0f, 12, FColor::Green, false, 2.0f);
    DrawDebugLine(GetWorld(), Target->GetActorLocation(), TargetPosition, FColor::Green, false, 2.0f, 0, 3.0f);
    #endif

    UE_LOG(LogTemp, Warning, TEXT("GravityPull: Pulled %s from %.1f to target distance %.1f"), 
        *Target->GetName(), CurrentDistance, TargetDistance);
}

bool UGravityPullAbilityComponent::CanExecute() const
{
    // Let base class handle all resource checks
    return Super::CanExecute();
}

AActor* UGravityPullAbilityComponent::FindBestHackableTarget() const
{
    // This function is deprecated - we now use trace-based targeting
    // Keeping for backward compatibility but it's not used
    return nullptr;
}


void UGravityPullAbilityComponent::GetCameraLocationAndDirection(FVector& OutLocation, FVector& OutDirection) const
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Try to get camera from character
    if (ACharacter* Character = Cast<ACharacter>(Owner))
    {
        UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
        if (Camera)
        {
            OutLocation = Camera->GetComponentLocation();
            OutDirection = Camera->GetForwardVector();
            return;
        }
    }

    // Fallback to actor location and rotation
    OutLocation = Owner->GetActorLocation();
    OutDirection = Owner->GetActorForwardVector();
}

bool UGravityPullAbilityComponent::IsWithinTargetingAngle(const FVector& CameraDirection, const FVector& ToTarget) const
{
    // This function is deprecated - we now use trace-based targeting
    // Keeping for backward compatibility
    return true;
}

void UGravityPullAbilityComponent::ExecuteUltimate()
{
    // Ultimate Gravity Pull - "Singularity"
    // Creates a black hole that pulls ALL enemies to a central point
    UE_LOG(LogTemp, Warning, TEXT("ULTIMATE GRAVITY PULL: Singularity!"));
    
    Super::ExecuteUltimate();
    
    AActor* Owner = GetOwner();
    if (!Owner) return;
    
    FVector CameraLocation, CameraDirection;
    GetCameraLocationAndDirection(CameraLocation, CameraDirection);
    
    // Create singularity point ahead of player
    FVector SingularityPoint = CameraLocation + (CameraDirection * UltimateSingularityDistance);
    
    // Ultimate version: Pull ALL enemies in massive radius
    float UltimateRadius = Range * UltimateRadiusMultiplier;
    float UltimatePullForce = BaseLaunchForce * UltimatePullForceMultiplier;
    
    // Find all actors in massive radius
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(Owner);
    
    TArray<AActor*> OutActors;
    // Create object types for detection - include all relevant types
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
    
    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        SingularityPoint,
        UltimateRadius,
        ObjectTypes,
        nullptr,
        ActorsToIgnore,
        OutActors
    );
    
    UE_LOG(LogTemp, Warning, TEXT("Singularity: Found %d actors in radius %.1f"), OutActors.Num(), UltimateRadius);
    
    int32 EntitiesPulled = 0;
    
    // Check for PsiDisruptors first and destroy them
    for (AActor* Actor : OutActors)
    {
        if (!Actor) continue;
        
        UE_LOG(LogTemp, Verbose, TEXT("Singularity checking actor: %s (Class: %s)"), 
            *Actor->GetName(), *Actor->GetClass()->GetName());
        
        // Check if it's a PsiDisruptor
        if (APsiDisruptor* Disruptor = Cast<APsiDisruptor>(Actor))
        {
            UE_LOG(LogTemp, Error, TEXT("SINGULARITY: Found Psi-Disruptor! Destroying it!"));
            Disruptor->DestroyByUltimate();
            EntitiesPulled++;
            continue;
        }
        else if (Actor->GetClass()->GetName().Contains(TEXT("PsiDisruptor")))
        {
            // Fallback check in case cast fails
            UE_LOG(LogTemp, Error, TEXT("SINGULARITY: Found PsiDisruptor by name but cast failed! Class: %s"), 
                *Actor->GetClass()->GetName());
        }
    }
    
    // Pull everything to the singularity point
    for (AActor* Actor : OutActors)
    {
        if (!Actor) continue;
        
        // Skip if already destroyed (like PsiDisruptor)
        if (!IsValid(Actor)) continue;
        
        // Check if it's an enemy or physics object
        bool bIsEnemy = Actor->ActorHasTag("Enemy") || Actor->IsA<ACharacter>();
        bool bIsPhysicsObject = Actor->ActorHasTag("Hackable") || Actor->FindComponentByClass<UPrimitiveComponent>();
        
        if (!bIsEnemy && !bIsPhysicsObject) continue;
        
        // Skip Tank enemies - they are immune
        if (Actor->IsA<ATankEnemy>())
        {
            UE_LOG(LogTemp, Warning, TEXT("Singularity: Tank enemy %s is immune to pull"), *Actor->GetName());
            continue;
        }
        
        // Calculate pull direction (towards singularity)
        FVector PullDirection = (SingularityPoint - Actor->GetActorLocation()).GetSafeNormal();
        
        // Apply pull force
        if (ACharacter* Character = Cast<ACharacter>(Actor))
        {
            // For characters, use launch with bounce
            FVector LaunchVelocity = PullDirection * UltimatePullForce;
            LaunchVelocity.Z += BounceUpwardForce * 0.7f; // Slightly less bounce for ultimate
            Character->LaunchCharacter(LaunchVelocity, true, true);
            EntitiesPulled++;
            
            UE_LOG(LogTemp, Warning, TEXT("Singularity pulled character %s!"), *Actor->GetName());
        }
        else if (UPrimitiveComponent* PrimComp = Actor->FindComponentByClass<UPrimitiveComponent>())
        {
            // For physics objects
            if (PrimComp->IsSimulatingPhysics())
            {
                PrimComp->AddImpulse(PullDirection * UltimatePullForce * PrimComp->GetMass());
                EntitiesPulled++;
                
                UE_LOG(LogTemp, Warning, TEXT("Singularity pulled object %s!"), *Actor->GetName());
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Ultimate Gravity Pull: Created singularity, pulled %d entities!"), EntitiesPulled);
    
    // Visual effect at singularity point
    #if WITH_EDITOR
    // Draw debug sphere at singularity
    DrawDebugSphere(GetWorld(), SingularityPoint, 200.0f, 32, FColor::Purple, false, 3.0f, 0, 5.0f);
    
    // Draw pull lines
    for (AActor* Actor : OutActors)
    {
        if (Actor && (Actor->ActorHasTag("Enemy") || Actor->ActorHasTag("Hackable")))
        {
            DrawDebugLine(GetWorld(), Actor->GetActorLocation(), SingularityPoint, 
                FColor::Purple, false, 2.0f, 0, 2.0f);
        }
    }
    #endif
}