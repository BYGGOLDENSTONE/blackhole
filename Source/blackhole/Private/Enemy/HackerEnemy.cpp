#include "Enemy/HackerEnemy.h"
#include "Components/Abilities/Enemy/MindmeldComponent.h"
#include "Player/BlackholePlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Systems/ResourceManager.h"
#include "Enemy/AI/HackerEnemyStateMachine.h"

AHackerEnemy::AHackerEnemy()
{
	// Replace base state machine with hacker-specific one
	StateMachine = CreateDefaultSubobject<UHackerEnemyStateMachine>(TEXT("HackerStateMachine"));
	
	MindmeldAbility = CreateDefaultSubobject<UMindmeldComponent>(TEXT("MindmeldAbility"));

	MindmeldRange = 3000.0f;
	SafeDistance = 400.0f; // Get closer
	
	// Configure hacker movement - cautious and ranged
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 400.0f; // Moderate speed
		Movement->MaxAcceleration = 500.0f; // Moderate acceleration
		Movement->BrakingDecelerationWalking = 800.0f; // Quick stops for positioning
		Movement->RotationRate = FRotator(0.0f, 300.0f, 0.0f); // Normal turning
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
	}
	
	// Set default data table row name
	StatsRowName = FName("Hacker");
	
	// Hacker enemies give high WP reward due to their danger
	WPRewardOnKill = 25.0f;
}

void AHackerEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}
}

void AHackerEnemy::UpdateAIBehavior(float DeltaTime)
{
	// Call parent implementation first to handle combat detection
	Super::UpdateAIBehavior(DeltaTime);
	
	if (!TargetActor || !IsAlive())
	{
		return;
	}

	// Make sure we're targeting the player
	if (!TargetActor->IsA<ABlackholePlayerCharacter>())
	{
		// Try to find the player
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		if (!TargetActor)
		{
			return;
		}
	}

	MaintainLineOfSight(DeltaTime);

	float Distance = GetDistanceToTarget();

	if (Distance <= MindmeldRange && HasLineOfSightToTarget())
	{
		if (!MindmeldAbility->bIsMindmeldActive)
		{
			// Start Mindmeld without logging
			MindmeldAbility->SetTarget(TargetActor);
			MindmeldAbility->Execute();
		}
	}
	else if (MindmeldAbility->bIsMindmeldActive)
	{
		// Stop mindmeld without logging every tick
		MindmeldAbility->StopMindmeld();
	}
}

void AHackerEnemy::MaintainLineOfSight(float DeltaTime)
{
	if (!TargetActor)
	{
		return;
	}

	float Distance = GetDistanceToTarget();
	FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();

	if (Distance < SafeDistance)
	{
		AddMovementInput(-Direction, 0.5f);
	}
	else if (Distance > MindmeldRange * 0.8f)
	{
		AddMovementInput(Direction, 0.5f);
	}

	FRotator NewRotation = Direction.Rotation();
	NewRotation.Pitch = 0.0f;
	NewRotation.Roll = 0.0f;
	SetActorRotation(NewRotation);
}

bool AHackerEnemy::HasLineOfSightToTarget() const
{
	if (!TargetActor)
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(TargetActor);

	FVector Start = GetActorLocation() + FVector(0, 0, 50);
	FVector End = TargetActor->GetActorLocation() + FVector(0, 0, 50);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);

	// If nothing was hit between us and target, we have line of sight
	return !bHit;
}

float AHackerEnemy::GetDistanceToTarget() const
{
	if (!TargetActor)
	{
		return FLT_MAX;
	}

	return FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
}

void AHackerEnemy::OnDeath()
{
	// Stop mindmeld before calling parent death function
	if (MindmeldAbility && MindmeldAbility->bIsMindmeldActive)
	{
		MindmeldAbility->StopMindmeld();
	}
	
	// Reduce player's WP when killing a hacker enemy
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			// Reduce WP by 10 for killing a hacker enemy
			float WPReduction = 10.0f;
			ResourceMgr->AddWillPower(-WPReduction);
			
			UE_LOG(LogTemp, Log, TEXT("HackerEnemy killed: Reduced player WP by %.0f"), WPReduction);
		}
	}
	
	// Call parent implementation for ragdoll and cleanup
	Super::OnDeath();
}