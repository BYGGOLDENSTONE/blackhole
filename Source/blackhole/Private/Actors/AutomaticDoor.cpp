#include "Actors/AutomaticDoor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"

AAutomaticDoor::AAutomaticDoor()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Create root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    
    // Create door mesh
    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(RootComponent);
    
    // Create proximity trigger (for detecting when player is near)
    ProximityTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ProximityTrigger"));
    ProximityTrigger->SetupAttachment(RootComponent);
    ProximityTrigger->SetBoxExtent(FVector(ProximityRange, ProximityRange, 200.0f));
    ProximityTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProximityTrigger->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    ProximityTrigger->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    ProximityTrigger->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    
    // Create inside trigger (for detecting when player is inside/past the door)
    InsideTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("InsideTrigger"));
    InsideTrigger->SetupAttachment(DoorMesh);
    InsideTrigger->SetBoxExtent(FVector(150.0f, 150.0f, 200.0f));
    InsideTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f)); // Centered on door
    InsideTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InsideTrigger->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    InsideTrigger->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    InsideTrigger->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void AAutomaticDoor::BeginPlay()
{
    Super::BeginPlay();
    
    // Store initial positions
    ClosedPosition = DoorMesh->GetRelativeLocation();
    OpenPosition = ClosedPosition + FVector(0.0f, 0.0f, DoorHeight);
    
    // Bind overlap events
    ProximityTrigger->OnComponentBeginOverlap.AddDynamic(this, &AAutomaticDoor::OnProximityBeginOverlap);
    ProximityTrigger->OnComponentEndOverlap.AddDynamic(this, &AAutomaticDoor::OnProximityEndOverlap);
    InsideTrigger->OnComponentBeginOverlap.AddDynamic(this, &AAutomaticDoor::OnInsideBeginOverlap);
    InsideTrigger->OnComponentEndOverlap.AddDynamic(this, &AAutomaticDoor::OnInsideEndOverlap);
    
    // Cache player reference
    CachedPlayer = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    
    UE_LOG(LogTemp, Warning, TEXT("AutomaticDoor: Initialized at %s, Height: %.1f, ProximityRange: %.1f"), 
        *GetActorLocation().ToString(), DoorHeight, ProximityRange);
}

void AAutomaticDoor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Check if player is looking at door when nearby
    if (bPlayerNearby && !bPlayerInside)
    {
        CheckPlayerProximityAndLook();
    }
    
    // Update door movement
    UpdateDoorMovement(DeltaTime);
    
    // Debug visualization
    #if WITH_EDITOR
    if (bPlayerNearby)
    {
        DrawDebugBox(GetWorld(), GetActorLocation(), ProximityTrigger->GetScaledBoxExtent(), 
            bPlayerLooking ? FColor::Green : FColor::Yellow, false, -1.0f, 0, 2.0f);
    }
    #endif
}

void AAutomaticDoor::UpdateDoorMovement(float DeltaTime)
{
    FVector CurrentPosition = DoorMesh->GetRelativeLocation();
    FVector TargetPosition = ClosedPosition;
    
    switch (CurrentState)
    {
        case EDoorState::Opening:
            TargetPosition = OpenPosition;
            if (FVector::Dist(CurrentPosition, OpenPosition) < 1.0f)
            {
                CurrentState = EDoorState::Open;
                DoorMesh->SetRelativeLocation(OpenPosition);
                UE_LOG(LogTemp, Warning, TEXT("Door fully opened"));
                return;
            }
            break;
            
        case EDoorState::Closing:
            TargetPosition = ClosedPosition;
            if (FVector::Dist(CurrentPosition, ClosedPosition) < 1.0f)
            {
                CurrentState = EDoorState::Closed;
                DoorMesh->SetRelativeLocation(ClosedPosition);
                UE_LOG(LogTemp, Warning, TEXT("Door fully closed"));
                return;
            }
            break;
            
        case EDoorState::Open:
        case EDoorState::Closed:
            return; // No movement needed
    }
    
    // Interpolate to target position
    FVector NewPosition = FMath::VInterpConstantTo(CurrentPosition, TargetPosition, DeltaTime, MoveSpeed);
    DoorMesh->SetRelativeLocation(NewPosition);
}

void AAutomaticDoor::CheckPlayerProximityAndLook()
{
    if (!CachedPlayer) return;
    
    bool bWasLooking = bPlayerLooking;
    bPlayerLooking = IsPlayerLookingAtDoor();
    
    // If player just started looking at door and it's closed, open it
    if (bPlayerLooking && !bWasLooking && CurrentState == EDoorState::Closed)
    {
        OpenDoor();
    }
    // If player stopped looking and door is open, start auto-close timer
    else if (!bPlayerLooking && bWasLooking && CurrentState == EDoorState::Open)
    {
        StartAutoCloseTimer();
    }
}

bool AAutomaticDoor::IsPlayerLookingAtDoor() const
{
    if (!CachedPlayer) return false;
    
    // Get player's view direction
    FVector PlayerViewLocation;
    FRotator PlayerViewRotation;
    CachedPlayer->GetActorEyesViewPoint(PlayerViewLocation, PlayerViewRotation);
    FVector PlayerLookDirection = PlayerViewRotation.Vector();
    
    // Get direction from player to door
    FVector PlayerToDoor = (GetActorLocation() - PlayerViewLocation).GetSafeNormal();
    
    // Calculate dot product
    float DotProduct = FVector::DotProduct(PlayerLookDirection, PlayerToDoor);
    
    // Check if player is looking at door (within threshold)
    bool bLookingAt = DotProduct >= LookAtThreshold;
    
    
    return bLookingAt;
}

void AAutomaticDoor::OpenDoor()
{
    if (CurrentState == EDoorState::Open || CurrentState == EDoorState::Opening) return;
    
    CurrentState = EDoorState::Opening;
    CancelAutoCloseTimer();
    
    UE_LOG(LogTemp, Warning, TEXT("Door opening - Player is nearby and looking at door"));
}

void AAutomaticDoor::CloseDoor()
{
    if (CurrentState == EDoorState::Closed || CurrentState == EDoorState::Closing) return;
    
    // Don't close if player is inside
    if (bPlayerInside)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot close door - player is inside"));
        return;
    }
    
    CurrentState = EDoorState::Closing;
    CancelAutoCloseTimer();
    
    UE_LOG(LogTemp, Warning, TEXT("Door closing"));
}

void AAutomaticDoor::StartAutoCloseTimer()
{
    if (AutoCloseDelay > 0.0f && !bPlayerInside)
    {
        GetWorld()->GetTimerManager().SetTimer(AutoCloseTimer, this, &AAutomaticDoor::CloseDoor, AutoCloseDelay, false);
        UE_LOG(LogTemp, Warning, TEXT("Auto-close timer started (%.1fs)"), AutoCloseDelay);
    }
}

void AAutomaticDoor::CancelAutoCloseTimer()
{
    if (GetWorld()->GetTimerManager().IsTimerActive(AutoCloseTimer))
    {
        GetWorld()->GetTimerManager().ClearTimer(AutoCloseTimer);
        UE_LOG(LogTemp, Warning, TEXT("Auto-close timer cancelled"));
    }
}

void AAutomaticDoor::OnProximityBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(OtherActor))
    {
        bPlayerNearby = true;
        CachedPlayer = Player;
        
        UE_LOG(LogTemp, Warning, TEXT("Player entered door proximity"));
    }
}

void AAutomaticDoor::OnProximityEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (Cast<ABlackholePlayerCharacter>(OtherActor))
    {
        bPlayerNearby = false;
        bPlayerLooking = false;
        
        // If door is open and player left proximity, start auto-close
        if (CurrentState == EDoorState::Open && !bPlayerInside)
        {
            StartAutoCloseTimer();
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Player left door proximity"));
    }
}

void AAutomaticDoor::OnInsideBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (Cast<ABlackholePlayerCharacter>(OtherActor))
    {
        bPlayerInside = true;
        CancelAutoCloseTimer(); // Don't close while player is inside
        
        UE_LOG(LogTemp, Warning, TEXT("Player entered inside door area"));
    }
}

void AAutomaticDoor::OnInsideEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (Cast<ABlackholePlayerCharacter>(OtherActor))
    {
        bPlayerInside = false;
        
        // Close door immediately when player exits the inside area
        CloseDoor();
        
        UE_LOG(LogTemp, Warning, TEXT("Player left inside door area - closing door"));
    }
}