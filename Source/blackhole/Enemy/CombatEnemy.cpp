#include "CombatEnemy.h"
#include "../Components/Attributes/IntegrityComponent.h"
#include "../Components/Abilities/SmashAbilityComponent.h"
#include "../Components/Abilities/BlockComponent.h"
#include "../Components/Abilities/DodgeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ACombatEnemy::ACombatEnemy()
{
	SmashAbility = CreateDefaultSubobject<USmashAbilityComponent>(TEXT("SmashAbility"));
	BlockAbility = CreateDefaultSubobject<UBlockComponent>(TEXT("BlockAbility"));
	DodgeAbility = CreateDefaultSubobject<UDodgeComponent>(TEXT("DodgeAbility"));

	AttackRange = 150.0f;
	ChaseRange = 1000.0f;
}

void ACombatEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}
}

void ACombatEnemy::UpdateAIBehavior(float DeltaTime)
{
	if (!TargetActor || !IntegrityComponent->IsAlive())
	{
		return;
	}

	float Distance = GetDistanceToTarget();

	if (Distance <= AttackRange)
	{
		TryAttack();
	}
	else if (Distance <= ChaseRange)
	{
		MoveTowardsTarget(DeltaTime);
	}
}

void ACombatEnemy::MoveTowardsTarget(float DeltaTime)
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

void ACombatEnemy::TryAttack()
{
	if (SmashAbility && SmashAbility->CanExecute())
	{
		SmashAbility->Execute();
	}
}

float ACombatEnemy::GetDistanceToTarget() const
{
	if (!TargetActor)
	{
		return FLT_MAX;
	}

	return FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
}