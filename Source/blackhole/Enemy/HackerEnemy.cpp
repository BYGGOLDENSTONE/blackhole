#include "HackerEnemy.h"
#include "../Components/Attributes/IntegrityComponent.h"
#include "../Components/Abilities/MindmeldComponent.h"
#include "../Player/BlackholePlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AHackerEnemy::AHackerEnemy()
{
	MindmeldAbility = CreateDefaultSubobject<UMindmeldComponent>(TEXT("MindmeldAbility"));

	MindmeldRange = 3000.0f;
	SafeDistance = 500.0f;
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
	if (!TargetActor || !IntegrityComponent->IsAlive())
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
			UE_LOG(LogTemp, Warning, TEXT("HackerEnemy: Starting Mindmeld on %s"), *TargetActor->GetName());
			MindmeldAbility->SetTarget(TargetActor);
			MindmeldAbility->Execute();
		}
	}
	else if (MindmeldAbility->bIsMindmeldActive)
	{
		if (Distance > MindmeldRange)
		{
			UE_LOG(LogTemp, Warning, TEXT("HackerEnemy: Target out of range (%.1f > %.1f)"), Distance, MindmeldRange);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("HackerEnemy: Lost line of sight"));
		}
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