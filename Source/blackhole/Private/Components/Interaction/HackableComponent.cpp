#include "Components/Interaction/HackableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

UHackableComponent::UHackableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHackableComponent::BeginPlay()
{
    Super::BeginPlay();

    // Find the mesh component on the owner
    AActor* Owner = GetOwner();
    if (Owner)
    {
        MeshComponent = Owner->FindComponentByClass<UPrimitiveComponent>();
        if (MeshComponent && MeshComponent->GetMaterial(0))
        {
            OriginalMaterial = MeshComponent->GetMaterial(0);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("HackableComponent BeginPlay - LaunchDirection set to: %d"), (int32)LaunchDirection);
}

void UHackableComponent::OnTargeted()
{
    if (!bIsHackable || !MeshComponent) return;

    // Apply highlight material
    if (HighlightMaterial)
    {
        MeshComponent->SetMaterial(0, HighlightMaterial);
    }

    if (AActor* Owner = GetOwner())
    {
        UE_LOG(LogTemp, Warning, TEXT("Hackable object targeted: %s"), *Owner->GetName());
    }
}

void UHackableComponent::OnUntargeted()
{
    if (!MeshComponent) return;

    // Restore original material
    if (OriginalMaterial)
    {
        MeshComponent->SetMaterial(0, OriginalMaterial);
    }
}

void UHackableComponent::OnHacked(float LaunchForce, const FVector& PlayerLocation)
{
    if (!bIsHackable || !MeshComponent) return;

    // Temporarily disable further hacking
    bIsHackable = false;

    // Enable physics if not already enabled
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    
    // Constrain physics to prevent drift and tumbling
    MeshComponent->SetLinearDamping(2.0f);     // Reduce sliding
    MeshComponent->SetAngularDamping(10.0f);   // Stop tumbling
    
    // Lock rotations to keep object upright
    MeshComponent->BodyInstance.bLockXRotation = true;
    MeshComponent->BodyInstance.bLockYRotation = true;
    MeshComponent->BodyInstance.bLockZRotation = true;
    MeshComponent->RecreatePhysicsState();
    
    // Get launch direction based on component settings and player location
    FVector Direction = GetLaunchDirection(PlayerLocation);
    
    // Apply impulse based on direction and force
    // The physics engine already accounts for mass, so we don't multiply by it
    FVector Impulse = Direction * LaunchForce;
    MeshComponent->AddImpulse(Impulse, NAME_None, true);

    // Restore original material
    OnUntargeted();

    // Reset hackable state after 5 seconds
    GetWorld()->GetTimerManager().SetTimer(ResetHackableTimer, this, 
        &UHackableComponent::ResetHackableState, 5.0f, false);

    if (AActor* Owner = GetOwner())
    {
        UE_LOG(LogTemp, Warning, TEXT("Hackable object launched: %s with force %f in direction %s"), 
            *Owner->GetName(), LaunchForce, *Direction.ToString());
    }
}

FVector UHackableComponent::GetLaunchDirection(const FVector& PlayerLocation) const
{
    AActor* Owner = GetOwner();
    if (!Owner) return FVector::UpVector;
    
    FVector ObjectLocation = Owner->GetActorLocation();
    FVector PlayerToObject = ObjectLocation - PlayerLocation;
    
    UE_LOG(LogTemp, Warning, TEXT("GetLaunchDirection - LaunchDirection: %d"), (int32)LaunchDirection);
    UE_LOG(LogTemp, Warning, TEXT("Player Location: %s"), *PlayerLocation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("Object Location: %s"), *ObjectLocation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("PlayerToObject: %s"), *PlayerToObject.ToString());
    
    switch (LaunchDirection)
    {
        case EHackableLaunchDirection::XAxis:
        {
            // X axis in Unreal: Forward/Backward
            // If object is forward of player (positive X), launch backward (-X)
            // If object is behind player (negative X), launch forward (+X)
            if (PlayerToObject.X > 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("X-Axis: Object is forward of player, launching BACKWARD"));
                return FVector(-1, 0, 0);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("X-Axis: Object is behind player, launching FORWARD"));
                return FVector(1, 0, 0);
            }
        }
            
        case EHackableLaunchDirection::YAxis:
        {
            // Y axis in Unreal: Right/Left (side-to-side)
            // If object is to the right of player (positive Y), launch left (-Y)
            // If object is to the left of player (negative Y), launch right (+Y)
            if (PlayerToObject.Y > 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("Y-Axis: Object is right of player, launching LEFT"));
                return FVector(0, -1, 0);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Y-Axis: Object is left of player, launching RIGHT"));
                return FVector(0, 1, 0);
            }
        }
            
        case EHackableLaunchDirection::ZAxis:
        {
            // If object is above player (positive Z), launch down (-Z)
            // If object is below player (negative Z), launch up (+Z)
            if (PlayerToObject.Z > 0)
                return FVector(0, 0, -1); // Object is above, launch down
            else
                return FVector(0, 0, 1);  // Object is below, launch up
        }
            
        case EHackableLaunchDirection::Auto:
        default:
            // For auto mode, launch away from player in all directions
            UE_LOG(LogTemp, Warning, TEXT("Auto mode: Launching away from player"));
            return PlayerToObject.GetSafeNormal();
    }
}

void UHackableComponent::ResetHackableState()
{
    bIsHackable = true;
    
    // Optionally reset physics state
    if (MeshComponent)
    {
        // You might want to keep physics enabled or disable based on your needs
        // MeshComponent->SetSimulatePhysics(false);
    }
}