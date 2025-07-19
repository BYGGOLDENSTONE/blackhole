#include "Actors/GravityShiftVolume.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/Character.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Enemy/BaseEnemy.h"
#include "DrawDebugHelpers.h"

AGravityShiftVolume::AGravityShiftVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create trigger volume
    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
    RootComponent = TriggerVolume;
    TriggerVolume->SetBoxExtent(FVector(500, 500, 500));
    TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    TriggerVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    TriggerVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

    // Create arrow to show gravity direction
    GravityDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("GravityDirectionArrow"));
    GravityDirectionArrow->SetupAttachment(RootComponent);
    GravityDirectionArrow->SetArrowColor(FColor::Blue);
    GravityDirectionArrow->ArrowSize = 2.0f;
    GravityDirectionArrow->SetRelativeRotation(FRotator(-90, 0, 0)); // Default points down
}

void AGravityShiftVolume::BeginPlay()
{
    Super::BeginPlay();

    // Bind overlap events
    TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AGravityShiftVolume::OnOverlapBegin);
    TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AGravityShiftVolume::OnOverlapEnd);

    // Update arrow to show gravity direction
    FVector GravityDir = bUseCustomDirection ? CustomGravityDirection : FVector(0, 0, -1);
    
    switch (GravityAxis)
    {
        case EGravityAxis::Left:
            GravityDir = FVector(-1, 0, 0);
            break;
        case EGravityAxis::Right:
            GravityDir = FVector(1, 0, 0);
            break;
        case EGravityAxis::Forward:
            GravityDir = FVector(0, 1, 0);
            break;
        case EGravityAxis::Backward:
            GravityDir = FVector(0, -1, 0);
            break;
        case EGravityAxis::Up:
            GravityDir = FVector(0, 0, 1);
            break;
    }

    if (!bUseCustomDirection)
    {
        CustomGravityDirection = GravityDir;
    }

    // Point arrow in gravity direction
    FRotator ArrowRotation = GravityDir.Rotation();
    ArrowRotation.Pitch -= 90; // Arrow component points up by default
    GravityDirectionArrow->SetWorldRotation(ArrowRotation);
}

void AGravityShiftVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor) return;

    // Check if we should affect this actor
    if (bOnlyAffectPlayers && !OtherActor->IsA<ABlackholePlayerCharacter>())
    {
        return;
    }

    // Only affect characters
    if (!OtherActor->IsA<ACharacter>())
    {
        return;
    }

    ApplyGravityToActor(OtherActor);
}

void AGravityShiftVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || !bRestoreOnExit) return;

    RestoreGravityForActor(OtherActor);
}

void AGravityShiftVolume::ApplyGravityToActor(AActor* Actor)
{
    if (!Actor) return;

    // Get or create gravity component
    UGravityDirectionComponent* GravityComp = Actor->FindComponentByClass<UGravityDirectionComponent>();
    if (!GravityComp)
    {
        GravityComp = NewObject<UGravityDirectionComponent>(Actor);
        GravityComp->RegisterComponent();
    }

    // Track affected actors
    AffectedActors.Add(Actor, GravityComp);

    // Apply gravity shift
    if (bUseCustomDirection)
    {
        GravityComp->TransitionToGravityDirection(CustomGravityDirection, TransitionDuration);
    }
    else
    {
        GravityComp->SetGravityAxis(GravityAxis);
    }

    UE_LOG(LogTemp, Log, TEXT("GravityShiftVolume: Applied gravity shift to %s"), *Actor->GetName());

    #if WITH_EDITOR
    // Debug visualization
    DrawDebugLine(GetWorld(), Actor->GetActorLocation(), 
        Actor->GetActorLocation() + (CustomGravityDirection * 300.0f), 
        FColor::Blue, false, TransitionDuration + 1.0f, 0, 5.0f);
    #endif
}

void AGravityShiftVolume::RestoreGravityForActor(AActor* Actor)
{
    if (!Actor) return;

    UGravityDirectionComponent** GravityCompPtr = AffectedActors.Find(Actor);
    if (GravityCompPtr && *GravityCompPtr)
    {
        // Restore to default gravity
        (*GravityCompPtr)->TransitionToGravityDirection(FVector(0, 0, -1), TransitionDuration);
        
        UE_LOG(LogTemp, Log, TEXT("GravityShiftVolume: Restored gravity for %s"), *Actor->GetName());
    }

    AffectedActors.Remove(Actor);
}