#include "Actors/PsiDisruptor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Abilities/Player/Utility/HackerDashAbility.h"
#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "Components/Movement/WallRunComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

APsiDisruptor::APsiDisruptor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Create base mesh
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
	BaseMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BaseMesh->SetCollisionResponseToAllChannels(ECR_Block);
	
	// Create core mesh (visual effect)
	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	CoreMesh->SetupAttachment(BaseMesh);
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Create disruption field
	DisruptionField = CreateDefaultSubobject<USphereComponent>(TEXT("DisruptionField"));
	DisruptionField->SetupAttachment(RootComponent);
	DisruptionField->SetSphereRadius(DisruptionRadius);
	DisruptionField->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DisruptionField->SetCollisionResponseToAllChannels(ECR_Ignore);
	DisruptionField->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void APsiDisruptor::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind overlap events
	DisruptionField->OnComponentBeginOverlap.AddDynamic(this, &APsiDisruptor::OnDisruptionFieldBeginOverlap);
	DisruptionField->OnComponentEndOverlap.AddDynamic(this, &APsiDisruptor::OnDisruptionFieldEndOverlap);
	
	// Start disruption effect
	GetWorld()->GetTimerManager().SetTimer(DisruptionTickHandle, this, &APsiDisruptor::ApplyDisruption, 0.5f, true);
	
	// Alert player about the disruptor
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		// TODO: Add UI notification
		UE_LOG(LogTemp, Warning, TEXT("PSI-DISRUPTOR ACTIVE: Movement abilities disabled in area!"));
	}
}

void APsiDisruptor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DisruptionTickHandle);
	}
	
	// Remove disruption from all affected players
	RemoveDisruption();
	
	Super::EndPlay(EndPlayReason);
}

void APsiDisruptor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Visual effect - rotate core
	if (CoreMesh)
	{
		FRotator NewRotation = CoreMesh->GetRelativeRotation();
		NewRotation.Yaw += DeltaTime * 45.0f;
		CoreMesh->SetRelativeRotation(NewRotation);
	}
	
	#if WITH_EDITOR
	// Debug visualization
	DrawDebugSphere(GetWorld(), GetActorLocation(), DisruptionRadius, 32, FColor::Purple, false, -1.0f, 0, 2.0f);
	#endif
}

float APsiDisruptor::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
	class AController* EventInstigator, AActor* DamageCauser)
{
	// Psi-Disruptor is invulnerable to normal damage
	// Can only be destroyed by singularity ability
	UE_LOG(LogTemp, Warning, TEXT("Psi-Disruptor is immune to normal damage! Use Singularity (Ultimate Gravity Pull) to destroy it."));
	return 0.0f;
}

void APsiDisruptor::DestroyByUltimate()
{
	UE_LOG(LogTemp, Warning, TEXT("Psi-Disruptor destroyed by Gravity Pull Ultimate!"));
	
	// Play destruction effect
	// TODO: Add particle effect
	
	RemoveDisruption();
	Destroy();
}

void APsiDisruptor::ApplyDisruption()
{
	// Visual pulse effect
	#if WITH_EDITOR
	DrawDebugSphere(GetWorld(), GetActorLocation(), DisruptionRadius * 1.1f, 32, FColor::Red, false, 0.5f);
	#endif
}

void APsiDisruptor::RemoveDisruption()
{
	// Re-enable abilities for all affected players
	for (AActor* Actor : AffectedPlayers)
	{
		if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(Actor))
		{
			// Re-enable movement abilities
			if (UHackerDashAbility* Dash = Player->FindComponentByClass<UHackerDashAbility>())
			{
				Dash->SetDisabled(false);
			}
			if (UHackerJumpAbility* Jump = Player->FindComponentByClass<UHackerJumpAbility>())
			{
				Jump->SetDisabled(false);
			}
			if (UWallRunComponent* WallRun = Player->FindComponentByClass<UWallRunComponent>())
			{
				WallRun->SetComponentTickEnabled(true);
			}
			
			UE_LOG(LogTemp, Warning, TEXT("Movement abilities restored for player"));
		}
	}
	
	AffectedPlayers.Empty();
}

void APsiDisruptor::OnDisruptionFieldBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(OtherActor))
	{
		// Disable movement abilities
		if (UHackerDashAbility* Dash = Player->FindComponentByClass<UHackerDashAbility>())
		{
			Dash->SetDisabled(true);
		}
		if (UHackerJumpAbility* Jump = Player->FindComponentByClass<UHackerJumpAbility>())
		{
			Jump->SetDisabled(true);
		}
		if (UWallRunComponent* WallRun = Player->FindComponentByClass<UWallRunComponent>())
		{
			WallRun->SetComponentTickEnabled(false);
		}
		
		AffectedPlayers.AddUnique(OtherActor);
		
		UE_LOG(LogTemp, Warning, TEXT("Player entered disruption field - movement abilities disabled!"));
	}
}

void APsiDisruptor::OnDisruptionFieldEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(OtherActor))
	{
		// Re-enable movement abilities
		if (UHackerDashAbility* Dash = Player->FindComponentByClass<UHackerDashAbility>())
		{
			Dash->SetDisabled(false);
		}
		if (UHackerJumpAbility* Jump = Player->FindComponentByClass<UHackerJumpAbility>())
		{
			Jump->SetDisabled(false);
		}
		if (UWallRunComponent* WallRun = Player->FindComponentByClass<UWallRunComponent>())
		{
			WallRun->SetComponentTickEnabled(true);
		}
		
		AffectedPlayers.Remove(OtherActor);
		
		UE_LOG(LogTemp, Warning, TEXT("Player left disruption field - movement abilities restored!"));
	}
}