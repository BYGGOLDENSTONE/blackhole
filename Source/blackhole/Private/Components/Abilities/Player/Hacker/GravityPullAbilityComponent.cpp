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
    Cost = 15.0f; // Legacy field
    StaminaCost = 10.0f; // New dual resource system
    WPCost = 15.0f; // New dual resource system
    HeatCost = 0.0f; // Hacker abilities don't consume heat
    Cooldown = 3.0f;       // 3 second cooldown
    Range = 2000.0f;       // 20 meter range
    
    // Default force values (will be applied as impulse)
    LaunchForceMultiplier = 1.0f;
    BaseLaunchForce = 50000.0f;  // Impulse needs to be higher since mass is accounted for by physics
}

void UGravityPullAbilityComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UGravityPullAbilityComponent::Execute()
{
    if (!CanExecute()) return;

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

    // Call base class to handle resource costs (stamina + WP corruption) and cooldown
    Super::Execute();

    // Calculate final launch force
    // Don't multiply by object mass - the physics engine already accounts for mass
    float FinalLaunchForce = BaseLaunchForce * LaunchForceMultiplier;

    // Get player location
    FVector PlayerLocation = GetOwner()->GetActorLocation();

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