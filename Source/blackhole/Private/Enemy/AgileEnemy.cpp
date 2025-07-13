#include "Enemy/AgileEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/DodgeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Enemy/EnemyUtility.h"
#include "Enemy/AI/AgileEnemyStateMachine.h"

AAgileEnemy::AAgileEnemy()
{
	// Replace base state machine with agile-specific one
	StateMachine = CreateDefaultSubobject<UAgileEnemyStateMachine>(TEXT("AgileStateMachine"));
	
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
	// Call parent implementation first to handle combat detection
	Super::UpdateAIBehavior(DeltaTime);
	
	if (!TargetActor || !IntegrityComponent->IsAlive())
	{
		return;
	}

	float Distance = UEnemyUtility::GetDistanceToTarget(this, TargetActor);

	// When player is close, randomly decide to dodge
	// Note: DodgeChance should be evaluated per decision, not frame-dependent
	if (Distance <= 300.0f && FMath::FRand() < DodgeChance)
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
	// Now handled by UEnemyUtility::MoveTowardsTarget
	UEnemyUtility::MoveTowardsTarget(this, TargetActor, DeltaTime, 450.0f);
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
	return UEnemyUtility::GetDistanceToTarget(this, TargetActor);
}