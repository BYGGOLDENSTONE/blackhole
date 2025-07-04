#include "AgileEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Abilities/Player/SmashAbilityComponent.h"
#include "Components/Abilities/Player/DodgeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AAgileEnemy::AAgileEnemy()
{
	// Only create the abilities this enemy type should have
	SmashAbility = CreateDefaultSubobject<USmashAbilityComponent>(TEXT("SmashAbility"));
	DodgeAbility = CreateDefaultSubobject<UDodgeComponent>(TEXT("DodgeAbility"));
	// NO BlockAbility - this enemy cannot block!

	AttackRange = 150.0f;
	ChaseRange = 1000.0f;
	DodgeChance = 0.3f; // 30% chance to dodge when attacked
}

void AAgileEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}
}

void AAgileEnemy::UpdateAIBehavior(float DeltaTime)
{
	if (!TargetActor || !IntegrityComponent->IsAlive())
	{
		return;
	}

	float Distance = GetDistanceToTarget();

	// When player is close, randomly decide to dodge
	if (Distance <= 300.0f && FMath::FRand() < DodgeChance * DeltaTime)
	{
		TryDodge();
	}

	if (Distance <= AttackRange)
	{
		TryAttack();
	}
	else if (Distance <= ChaseRange)
	{
		MoveTowardsTarget(DeltaTime);
	}
}

void AAgileEnemy::MoveTowardsTarget(float DeltaTime)
{
	if (!TargetActor)
	{
		return;
	}

	FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	AddMovementInput(Direction, 1.0f);

	FRotator NewRotation = Direction.Rotation();
	NewRotation.Pitch = 0.0f;
	NewRotation.Roll = 0.0f;
	SetActorRotation(NewRotation);
}

void AAgileEnemy::TryAttack()
{
	if (SmashAbility && SmashAbility->CanExecute())
	{
		SmashAbility->Execute();
	}
}

void AAgileEnemy::TryDodge()
{
	if (DodgeAbility && DodgeAbility->CanExecute())
	{
		DodgeAbility->Execute();
	}
}

float AAgileEnemy::GetDistanceToTarget() const
{
	if (!TargetActor)
	{
		return FLT_MAX;
	}

	return FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
}