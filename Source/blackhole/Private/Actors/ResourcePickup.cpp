#include "Actors/ResourcePickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Systems/ResourceManager.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/ErrorHandling.h"

AResourcePickup::AResourcePickup()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(50.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	// Create mesh
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(RootComponent);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Default values
	PickupType = EPickupType::WillPower;
	RestoreAmount = 20.0f;
	RespawnTime = 30.0f;
}

void AResourcePickup::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind overlap event
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AResourcePickup::OnOverlapBegin);
	
	// Get resource manager
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			ResourceManager = GameInstance->GetSubsystem<UResourceManager>();
			if (!ResourceManager)
			{
				UE_LOG(LogTemp, Error, TEXT("ResourcePickup: Failed to get ResourceManager!"));
			}
		}
	}
	
	// Store initial Z position for bobbing
	InitialZ = GetActorLocation().Z;
	
	// Spawn idle effect
	if (IdleEffect)
	{
		ActiveIdleEffect = UGameplayStatics::SpawnEmitterAttached(
			IdleEffect,
			PickupMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}
	
	// Set material based on pickup type
	if (PickupMesh && PickupMesh->GetStaticMesh())
	{
		// This could be expanded to set different materials for different pickup types
		// For now, we'll let the Blueprint handle the visual differences
	}
}

void AResourcePickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if player
	ABlackholePlayerCharacter* PlayerCharacter = Cast<ABlackholePlayerCharacter>(OtherActor);
	CHECK_CAST_VOID(PlayerCharacter);
	
	// Apply pickup effect
	ApplyPickupEffect(PlayerCharacter);
	
	// Hide pickup
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	
	// Set respawn timer if applicable
	if (RespawnTime > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(RespawnTimerHandle, this, &AResourcePickup::RespawnPickup, RespawnTime, false);
		}
	}
	else
	{
		// Destroy if no respawn
		Destroy();
	}
}

void AResourcePickup::ApplyPickupEffect(AActor* TargetActor)
{
	if (!ResourceManager)
	{
		UE_LOG(LogTemp, Error, TEXT("ResourcePickup: ResourceManager is null in ApplyPickupEffect!"));
		return;
	}
	
	// Spawn pickup effect
	if (PickupEffect && GetWorld())
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			PickupEffect,
			GetActorLocation(),
			FRotator::ZeroRotator,
			FVector(1.0f)
		);
	}
	
	// Play pickup sound
	if (PickupSound && GetWorld())
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickupSound, GetActorLocation());
	}
	
	// Stop idle effect
	if (ActiveIdleEffect)
	{
		ActiveIdleEffect->DeactivateSystem();
	}
	
	switch (PickupType)
	{
		case EPickupType::WillPower:
			ResourceManager->AddWillPower(RestoreAmount);
			UE_LOG(LogTemp, Log, TEXT("Pickup: Restored %.0f WillPower"), RestoreAmount);
			break;
			
		case EPickupType::HeatVent:
			// Heat system removed - convert heat vents to WP pickups
			ResourceManager->AddWillPower(RestoreAmount);
			UE_LOG(LogTemp, Log, TEXT("Pickup: Restored %.0f WillPower (Heat system removed)"), RestoreAmount);
			break;
	}
}

void AResourcePickup::RespawnPickup()
{
	// Show pickup again
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	
	// Restart idle effect
	if (ActiveIdleEffect)
	{
		ActiveIdleEffect->ActivateSystem();
	}
	
	UE_LOG(LogTemp, Log, TEXT("Pickup: Respawned"));
}

void AResourcePickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Rotate pickup
	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += RotationSpeed * DeltaTime;
	SetActorRotation(NewRotation);
	
	// Bob up and down
	if (UWorld* World = GetWorld())
	{
		FVector NewLocation = GetActorLocation();
		float BobOffset = FMath::Sin(World->GetTimeSeconds() * BobSpeed) * BobHeight;
		NewLocation.Z = InitialZ + BobOffset;
		SetActorLocation(NewLocation);
	}
}