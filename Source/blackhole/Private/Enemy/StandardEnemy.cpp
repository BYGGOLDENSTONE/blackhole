#include "Enemy/StandardEnemy.h"
#include "Components/Abilities/Enemy/SwordAttackComponent.h"
#include "Components/Abilities/Enemy/BuilderComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Enemy/EnemyUtility.h"
#include "Enemy/AI/StandardEnemyStateMachine.h"
#include "Actors/PsiDisruptor.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Movement/WallRunComponent.h"

AStandardEnemy::AStandardEnemy()
{
	// Replace base state machine with standard-specific one
	StateMachine = CreateDefaultSubobject<UStandardEnemyStateMachine>(TEXT("StandardStateMachine"));
	
	// Create abilities
	SwordAttack = CreateDefaultSubobject<USwordAttackComponent>(TEXT("SwordAttack"));
	BlockAbility = CreateDefaultSubobject<UBlockComponent>(TEXT("BlockAbility"));
	BuilderComponent = CreateDefaultSubobject<UBuilderComponent>(TEXT("BuilderComponent"));
	
	// Configure sword attack
	if (SwordAttack)
	{
		SwordAttack->BaseDamage = 20.0f;
		SwordAttack->AttackRange = 180.0f;
		SwordAttack->AttackAngle = 60.0f;
		SwordAttack->AttackSpeed = 1.0f;
	}
	
	// Configure builder component
	if (BuilderComponent)
	{
		BuilderComponent->BuildTime = 20.0f;
		BuilderComponent->BuildRadius = 500.0f;
		BuilderComponent->MinBuildersRequired = 2;
		// PsiDisruptorClass should be set in Blueprint
	}
	
	// Configure movement settings - standard speed
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 450.0f;
		Movement->MaxAcceleration = 800.0f;
		Movement->BrakingDecelerationWalking = 800.0f;
		Movement->RotationRate = FRotator(0.0f, 270.0f, 0.0f);
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
	}
	
	// Set default data table row name
	StatsRowName = FName("Standard");
	
	// Standard enemies give moderate WP on kill
	WPRewardOnKill = 15.0f;
	
	// Standard health
	MaxWP = 100.0f;
}

void AStandardEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}
	
	CurrentWP = MaxWP;
	
	// Disable builder component if this enemy doesn't have builder ability
	if (BuilderComponent && !bHasBuilderAbility)
	{
		BuilderComponent->SetComponentTickEnabled(false);
		BuilderComponent->DestroyComponent();
		BuilderComponent = nullptr;
	}
}

void AStandardEnemy::UpdateAIBehavior(float DeltaTime)
{
	// State machine handles all AI behavior now
	Super::UpdateAIBehavior(DeltaTime);
	
	// Track player air/wall run time
	if (TargetActor)
	{
		if (ABlackholePlayerCharacter* PlayerChar = Cast<ABlackholePlayerCharacter>(TargetActor))
		{
			bool bIsInAir = false;
			bool bIsWallRunning = false;
			
			// Check if player is in air
			if (UCharacterMovementComponent* PlayerMovement = PlayerChar->GetCharacterMovement())
			{
				bIsInAir = PlayerMovement->IsFalling();
			}
			
			// Check if player is wall running
			if (UWallRunComponent* WallRunComp = PlayerChar->FindComponentByClass<UWallRunComponent>())
			{
				bIsWallRunning = WallRunComp->IsWallRunning();
			}
			
			// Update air/wall run timer
			if (bIsInAir || bIsWallRunning)
			{
				PlayerAirWallRunTime += DeltaTime;
				
				// Check if we should build due to excessive air/wall time
				if (PlayerAirWallRunTime >= AirWallRunBuildThreshold && BuilderComponent && bHasBuilderAbility)
				{
					UE_LOG(LogTemp, Warning, TEXT("%s: Player has been in air/wall run for %.1f seconds, initiating build!"), 
						*GetName(), PlayerAirWallRunTime);
					CheckForBuildOpportunity();
					PlayerAirWallRunTime = 0.0f; // Reset timer
				}
			}
			else
			{
				// Player is grounded, reset timer
				PlayerAirWallRunTime = 0.0f;
			}
		}
	}
}

void AStandardEnemy::OnAlerted()
{
	// When alerted, check if we should start building
	if (BuilderComponent && bHasBuilderAbility)
	{
		CheckForBuildOpportunity();
	}
}

void AStandardEnemy::MoveTowardsTarget(float DeltaTime)
{
	if (!BlockAbility->IsBlocking())
	{
		UEnemyUtility::MoveTowardsTarget(this, TargetActor, DeltaTime, MovementSpeed);
	}
}

void AStandardEnemy::TryAttack()
{
	if (SwordAttack && SwordAttack->CanExecute())
	{
		SwordAttack->Execute();
	}
}

void AStandardEnemy::TryBlock()
{
	if (BlockAbility && BlockAbility->CanExecute())
	{
		// Random chance to block
		if (FMath::RandRange(0.0f, 1.0f) <= BlockChance)
		{
			BlockAbility->Execute();
		}
	}
}

float AStandardEnemy::GetDistanceToTarget() const
{
	return UEnemyUtility::GetDistanceToTarget(this, TargetActor);
}

bool AStandardEnemy::IsPlayerAttacking() const
{
	// Simple proximity check
	return GetDistanceToTarget() < 250.0f;
}

void AStandardEnemy::CheckForBuildOpportunity()
{
	if (!BuilderComponent || BuilderComponent->IsBuilding()) return;
	
	// Check if we should join an existing build
	if (UBuilderComponent* NearbyLeader = UBuilderComponent::FindNearestBuildLeader(this, BuilderComponent->BuildRadius))
	{
		BuilderComponent->JoinBuild(NearbyLeader);
		UE_LOG(LogTemp, Warning, TEXT("%s joining existing build"), *GetName());
		return;
	}
	
	// Check if we should start a new build
	if (ShouldStartBuilding())
	{
		// Find a good build location (between enemies)
		TArray<AStandardEnemy*> NearbyStandardEnemies;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStandardEnemy::StaticClass(), 
			reinterpret_cast<TArray<AActor*>&>(NearbyStandardEnemies));
		
		FVector BuildLocation = GetActorLocation();
		int32 BuilderCount = 0;
		
		for (AStandardEnemy* Enemy : NearbyStandardEnemies)
		{
			if (Enemy && Enemy != this && Enemy->BuilderComponent && Enemy->bHasBuilderAbility)
			{
				float Distance = FVector::Dist(GetActorLocation(), Enemy->GetActorLocation());
				if (Distance <= BuilderComponent->BuildRadius)
				{
					BuildLocation += Enemy->GetActorLocation();
					BuilderCount++;
				}
			}
		}
		
		if (BuilderCount > 0)
		{
			BuildLocation /= (BuilderCount + 1); // Average position
			BuildLocation.Z = GetActorLocation().Z; // Keep at ground level
			
			BuilderComponent->InitiateBuild(BuildLocation);
			UE_LOG(LogTemp, Warning, TEXT("%s initiating build at %s"), *GetName(), *BuildLocation.ToString());
		}
	}
}

bool AStandardEnemy::ShouldStartBuilding() const
{
	// Don't build if already building or no builder component
	if (!BuilderComponent || BuilderComponent->IsBuilding()) return false;
	
	// Check if there's already a psi-disruptor
	TArray<AActor*> Disruptors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APsiDisruptor::StaticClass(), Disruptors);
	if (Disruptors.Num() > 0) return false;
	
	// Check if enough builders are nearby
	int32 NearbyBuilders = 0;
	TArray<AStandardEnemy*> AllStandardEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStandardEnemy::StaticClass(), 
		reinterpret_cast<TArray<AActor*>&>(AllStandardEnemies));
	
	for (AStandardEnemy* Enemy : AllStandardEnemies)
	{
		if (Enemy && Enemy != this && Enemy->BuilderComponent && Enemy->bHasBuilderAbility)
		{
			float Distance = FVector::Dist(GetActorLocation(), Enemy->GetActorLocation());
			if (Distance <= BuilderComponent->BuildRadius)
			{
				NearbyBuilders++;
			}
		}
	}
	
	// Always return true if triggered by air/wall run timer (player spent too much time in air)
	if (PlayerAirWallRunTime >= AirWallRunBuildThreshold)
	{
		return NearbyBuilders >= 1; // Only need 1 other builder nearby when triggered by air time
	}
	
	return NearbyBuilders >= (BuilderComponent->MinBuildersRequired - 1);
}