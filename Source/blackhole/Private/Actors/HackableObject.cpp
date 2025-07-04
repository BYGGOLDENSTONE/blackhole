#include "Actors/HackableObject.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AHackableObject::AHackableObject()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create mesh component as root
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Set default physics settings
    MeshComponent->SetSimulatePhysics(false);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    MeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
    MeshComponent->SetMassOverrideInKg(NAME_None, Mass);
    MeshComponent->SetLinearDamping(LinearDamping);
    MeshComponent->SetAngularDamping(AngularDamping);
    
    // Enable physics constraints
    MeshComponent->BodyInstance.bLockXTranslation = false;
    MeshComponent->BodyInstance.bLockYTranslation = false;
    MeshComponent->BodyInstance.bLockZTranslation = false;
    MeshComponent->BodyInstance.bLockXRotation = false;
    MeshComponent->BodyInstance.bLockYRotation = false;
    MeshComponent->BodyInstance.bLockZRotation = false;

    // Create hackable component
    HackableComponent = CreateDefaultSubobject<UHackableComponent>(TEXT("HackableComponent"));

    // Set default values
    Mass = 100.0f;
    LinearDamping = 0.01f;
    AngularDamping = 0.0f;
    bStartWithPhysics = false;
    bIsHackable = true;
    MaxHackDistance = 1000.0f;
}

void AHackableObject::BeginPlay()
{
    Super::BeginPlay();

    // Apply settings from properties
    if (MeshComponent)
    {
        // Set the mesh if specified
        if (StaticMesh)
        {
            MeshComponent->SetStaticMesh(StaticMesh);
        }

        // Apply physics settings
        MeshComponent->SetSimulatePhysics(bStartWithPhysics);
        MeshComponent->SetMassOverrideInKg(NAME_None, Mass);
        MeshComponent->SetLinearDamping(LinearDamping);
        MeshComponent->SetAngularDamping(AngularDamping);

        // Make sure collision is properly set up
        if (!MeshComponent->GetBodyInstance())
        {
            MeshComponent->RecreatePhysicsState();
        }
    }

    // Apply hackable settings
    if (HackableComponent)
    {
        HackableComponent->bIsHackable = bIsHackable;
        HackableComponent->MaxHackDistance = MaxHackDistance;
        HackableComponent->ObjectMass = Mass;
        HackableComponent->LaunchDirection = LaunchDirection;
        
        if (HighlightMaterial)
        {
            HackableComponent->HighlightMaterial = HighlightMaterial;
        }
    }
}

#if WITH_EDITOR
void AHackableObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

    // Update mesh in editor
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, StaticMesh))
    {
        if (MeshComponent && StaticMesh)
        {
            MeshComponent->SetStaticMesh(StaticMesh);
        }
    }
    // Update mass in editor
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, Mass))
    {
        if (MeshComponent)
        {
            MeshComponent->SetMassOverrideInKg(NAME_None, Mass);
        }
        if (HackableComponent)
        {
            HackableComponent->ObjectMass = Mass;
        }
    }
    // Update physics simulation in editor
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, bStartWithPhysics))
    {
        if (MeshComponent)
        {
            MeshComponent->SetSimulatePhysics(bStartWithPhysics);
        }
    }
    // Update damping in editor
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, LinearDamping) || 
             PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, AngularDamping))
    {
        if (MeshComponent)
        {
            MeshComponent->SetLinearDamping(LinearDamping);
            MeshComponent->SetAngularDamping(AngularDamping);
        }
    }
    // Update hackable settings in editor
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, bIsHackable) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, MaxHackDistance) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, LaunchDirection))
    {
        if (HackableComponent)
        {
            HackableComponent->bIsHackable = bIsHackable;
            HackableComponent->MaxHackDistance = MaxHackDistance;
            HackableComponent->LaunchDirection = LaunchDirection;
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AHackableObject, HighlightMaterial))
    {
        if (HackableComponent)
        {
            HackableComponent->HighlightMaterial = HighlightMaterial;
        }
    }
}
#endif