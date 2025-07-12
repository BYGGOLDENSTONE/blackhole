#include "Enemy/BaseEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Systems/ThresholdManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Config/GameplayConfig.h"
#include "Player/BlackholePlayerCharacter.h"

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
	
	// Set default AI update rate from config
	AIUpdateRate = GameplayConfig::Enemy::AI_UPDATE_RATE;
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
	
	// Bind to integrity component's OnReachedZero event
	if (IntegrityComponent)
	{
		IntegrityComponent->OnReachedZero.AddDynamic(this, &ABaseEnemy::OnDeath);
	}
	
	// Start AI update timer
	if (GetWorld() && !bIsDead)
	{
		GetWorld()->GetTimerManager().SetTimer(AIUpdateTimer, this, &ABaseEnemy::TimerUpdateAI, AIUpdateRate, true);
	}
}

void ABaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// AI updates are now handled by timer
	// This tick function is only kept for movement/animation purposes
}

void ABaseEnemy::UpdateAIBehavior(float DeltaTime)
{
	// Check if we can see the player and start combat if needed
	if (TargetActor && !bHasStartedCombat)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("%s: Checking combat start - Has target"), *GetName());
		// Check both distance and line of sight
		float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
		if (Distance < GameplayConfig::Enemy::DETECTION_RANGE) // Within detection range
		{
			// Check line of sight
			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.AddIgnoredActor(TargetActor);
			
			FVector Start = GetActorLocation() + FVector(0, 0, GameplayConfig::Enemy::SIGHT_HEIGHT_OFFSET);
			FVector End = TargetActor->GetActorLocation() + FVector(0, 0, GameplayConfig::Enemy::SIGHT_HEIGHT_OFFSET);
			
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
								GEngine->AddOnScreenDebugMessage(-1, GameplayConfig::Enemy::COMBAT_MESSAGE_DURATION, FColor::Red, TEXT("COMBAT STARTED!"));
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
	
	// Stop AI updates
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AIUpdateTimer);
	}
	
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
	ImpulseDirection.Z = GameplayConfig::Enemy::DEATH_IMPULSE_Z; // Add some upward force
	GetMesh()->AddImpulse(ImpulseDirection * GameplayConfig::Enemy::DEATH_IMPULSE_MAGNITUDE, NAME_None, true);
	
	// Destroy actor after delay
	SetLifeSpan(GameplayConfig::Enemy::CORPSE_LIFESPAN);
}

void ABaseEnemy::TimerUpdateAI()
{
	if (!bIsDead)
	{
		// Call the existing UpdateAIBehavior with a fixed delta time
		UpdateAIBehavior(AIUpdateRate);
	}
}

ABlackholePlayerCharacter* ABaseEnemy::GetTargetPlayer() const
{
	return Cast<ABlackholePlayerCharacter>(TargetActor);
}

void ABaseEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AIUpdateTimer);
	}
	
	Super::EndPlay(EndPlayReason);
}