#include "Enemy/BaseEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Systems/ThresholdManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABaseEnemy::ABaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	IntegrityComponent = CreateDefaultSubobject<UIntegrityComponent>(TEXT("Integrity"));
	
	// Tag all enemies so hack abilities can identify them
	Tags.Add(FName("Enemy"));
	
	bIsDead = false;
	
	// Create sword mesh component - all enemies have swords
	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sword"));
	SwordMesh->SetupAttachment(GetMesh(), FName("weaponsocket"));
	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordMesh->SetCastShadow(true);
}

void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	// Try to find player if no target set
	if (!TargetActor)
	{
		if (UWorld* World = GetWorld())
		{
			TargetActor = UGameplayStatics::GetPlayerCharacter(World, 0);
			if (TargetActor)
			{
				UE_LOG(LogTemp, Log, TEXT("%s: Found player target in BeginPlay"), *GetName());
			}
		}
	}
}

void ABaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bIsDead)
	{
		// Check if dead
		if (IntegrityComponent && IntegrityComponent->GetCurrentValue() <= 0.0f)
		{
			OnDeath();
		}
		else
		{
			UpdateAIBehavior(DeltaTime);
		}
	}
}

void ABaseEnemy::UpdateAIBehavior(float DeltaTime)
{
	// Check if we can see the player and start combat if needed
	if (TargetActor && !bHasStartedCombat)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("%s: Checking combat start - Has target"), *GetName());
		// Check both distance and line of sight
		float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
		if (Distance < 2500.0f) // Within detection range
		{
			// Check line of sight
			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.AddIgnoredActor(TargetActor);
			
			FVector Start = GetActorLocation() + FVector(0, 0, 50);
			FVector End = TargetActor->GetActorLocation() + FVector(0, 0, 50);
			
			bool bHasLineOfSight = !GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
			
			if (bHasLineOfSight)
			{
				// Start combat when first enemy sees player
				if (UWorld* World = GetWorld())
				{
					if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
					{
						if (!ThresholdMgr->IsInCombat())
						{
							ThresholdMgr->StartCombat();
							UE_LOG(LogTemp, Warning, TEXT("Combat Started - %s detected player!"), *GetName());
							
							// Show on screen message
							if (GEngine)
							{
								GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("COMBAT STARTED!"));
							}
						}
					}
				}
				bHasStartedCombat = true;
			}
		}
	}
}

AActor* ABaseEnemy::GetTargetActor() const
{
	return TargetActor;
}

void ABaseEnemy::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void ABaseEnemy::OnDeath()
{
	if (bIsDead)
	{
		return;
	}
	
	bIsDead = true;
	
	// Stop all movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	
	// Disable collision on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Enable ragdoll physics
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	
	// Add some impulse to make the death more dramatic
	FVector ImpulseDirection = GetActorLocation() - (TargetActor ? TargetActor->GetActorLocation() : GetActorLocation());
	ImpulseDirection.Normalize();
	ImpulseDirection.Z = 0.5f; // Add some upward force
	GetMesh()->AddImpulse(ImpulseDirection * 5000.0f, NAME_None, true);
	
	// Destroy actor after delay
	SetLifeSpan(10.0f);
}