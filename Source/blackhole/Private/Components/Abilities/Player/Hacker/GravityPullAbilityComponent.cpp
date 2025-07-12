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

UGravityPullAbilityComponent::UGravityPullAbilityComponent()
{
    // Set default values
    WPCost = 15.0f; // New dual resource system
    // HeatCost removed - heat system no longer exists
    Cooldown = 3.0f;       // 3 second cooldown
    Range = 2000.0f;       // 20 meter range
    
    // Default force values (will be applied as impulse)
    LaunchForceMultiplier = 1.0f;
    BaseLaunchForce = 50000.0f;  // Impulse needs to be higher since mass is accounted for by physics
    
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
    // Find best hackable target
    AActor* Target = FindBestHackableTarget();
    if (!Target) 
    {
        UE_LOG(LogTemp, Warning, TEXT("GravityPull: No valid target found"));
        return;
    }

    // Get hackable component
    UHackableComponent* HackableComp = Target->FindComponentByClass<UHackableComponent>();
    if (!HackableComp || !HackableComp->CanBeHacked())
    {
        UE_LOG(LogTemp, Warning, TEXT("GravityPull: Target is not hackable"));
        return;
    }

    // Calculate final launch force
    // Don't multiply by object mass - the physics engine already accounts for mass
    float FinalLaunchForce = BaseLaunchForce * LaunchForceMultiplier;

    // Get player location
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("GravityPull: No owner"));
        return;
    }
    FVector PlayerLocation = Owner->GetActorLocation();

    // Launch the object (it will determine direction based on player location)
    HackableComp->OnHacked(FinalLaunchForce, PlayerLocation);

    UE_LOG(LogTemp, Warning, TEXT("GravityPull: Launched %s"), *Target->GetName());
}

bool UGravityPullAbilityComponent::CanExecute() const
{
    // Let base class handle all resource checks
    return Super::CanExecute();
}

AActor* UGravityPullAbilityComponent::FindBestHackableTarget() const
{
    AActor* Owner = GetOwner();
    if (!Owner) return nullptr;

    FVector CameraLocation;
    FVector CameraDirection;
    GetCameraLocationAndDirection(CameraLocation, CameraDirection);

    // Find all actors with hackable component in range
    TArray<AActor*> FoundActors;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        Owner->GetActorLocation(),
        Range,
        ObjectTypes,
        nullptr,
        TArray<AActor*>(),
        FoundActors
    );

    AActor* BestTarget = nullptr;
    float BestScore = -1.0f;

    for (AActor* Actor : FoundActors)
    {
        if (!Actor || Actor == Owner) continue;

        // Check if actor has hackable component
        UHackableComponent* HackableComp = Actor->FindComponentByClass<UHackableComponent>();
        if (!HackableComp || !HackableComp->CanBeHacked()) continue;

        // Check line of sight
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Owner);

        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            CameraLocation,
            Actor->GetActorLocation(),
            ECC_Visibility,
            QueryParams
        );

        if (bHit && HitResult.GetActor() != Actor) continue;

        // Check if within targeting angle
        FVector ToTarget = (Actor->GetActorLocation() - CameraLocation).GetSafeNormal();
        if (!IsWithinTargetingAngle(CameraDirection, ToTarget)) continue;

        // Calculate score based on distance and angle
        float Distance = FVector::Dist(CameraLocation, Actor->GetActorLocation());
        float AngleDot = FVector::DotProduct(CameraDirection, ToTarget);
        float Score = AngleDot * (1.0f - (Distance / Range));

        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = Actor;
        }
    }

    // Highlight the target
    if (BestTarget)
    {
        UHackableComponent* HackableComp = BestTarget->FindComponentByClass<UHackableComponent>();
        if (HackableComp)
        {
            HackableComp->OnTargeted();
        }
    }

    return BestTarget;
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
    float DotProduct = FVector::DotProduct(CameraDirection, ToTarget);
    float AngleRadians = FMath::Acos(DotProduct);
    float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);
    
    return AngleDegrees <= MaxTargetingAngle;
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
    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        SingularityPoint,
        UltimateRadius,
        TArray<TEnumAsByte<EObjectTypeQuery>>(),
        nullptr,
        ActorsToIgnore,
        OutActors
    );
    
    int32 EntitiesPulled = 0;
    
    // Pull everything to the singularity point
    for (AActor* Actor : OutActors)
    {
        if (!Actor) continue;
        
        // Check if it's an enemy or physics object
        bool bIsEnemy = Actor->ActorHasTag("Enemy") || Actor->IsA<ACharacter>();
        bool bIsPhysicsObject = Actor->ActorHasTag("Hackable") || Actor->FindComponentByClass<UPrimitiveComponent>();
        
        if (!bIsEnemy && !bIsPhysicsObject) continue;
        
        // Calculate pull direction (towards singularity)
        FVector PullDirection = (SingularityPoint - Actor->GetActorLocation()).GetSafeNormal();
        
        // Apply pull force
        if (ACharacter* Character = Cast<ACharacter>(Actor))
        {
            // For characters, use launch
            Character->LaunchCharacter(PullDirection * UltimatePullForce, true, true);
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