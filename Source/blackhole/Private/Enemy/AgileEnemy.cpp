#include "Enemy/AgileEnemy.h"
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

	// Agile enemy stats - like a rogue with daggers
	AttackRange = 100.0f; // Very short range - dagger-like
	ChaseRange = 1200.0f; // Longer chase range
	DodgeChance = 0.4f; // 40% chance to dodge
	
	// Combat stats
	MovementSpeed = 600.0f; // Very fast
	AttackSpeedMultiplier = 1.5f; // 50% faster attacks
	DashCooldown = 2.0f; // More frequent dashes (was 3.0)
	DashBehindDistance = 250.0f; // Distance behind player after dash
	BackstabStaggerDuration = 1.5f; // Player stagger duration on backstab hit
	MaintainDistanceMin = 450.0f; // Closer minimum distance for more aggression
	MaintainDistanceMax = 550.0f; // Closer maximum distance for more aggression
	BackstabDamageMultiplier = 2.0f; // Double damage for backstab
	RetreatDuration = 3.0f; // How long to retreat after backstab
	
	// Set default data table row name
	StatsRowName = FName("Agile");
	
	// Agile enemies give moderate WP reward
	WPRewardOnKill = 15.0f;
	
	// Configure agile movement settings - will be set in BeginPlay with MovementSpeed
}

void AAgileEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}
	
	// Apply movement speed from editor
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = MovementSpeed;
		Movement->MaxAcceleration = 1200.0f; // Quick acceleration
		Movement->BrakingDecelerationWalking = 1200.0f; // Quick stops
		Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // Very fast turning for agility
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
	}
	
	// Attack speed multiplier is handled in the combat state's cooldowns
}

void AAgileEnemy::UpdateAIBehavior(float DeltaTime)
{
	// Call parent implementation first to handle combat detection
	Super::UpdateAIBehavior(DeltaTime);
	
	if (!TargetActor || !IsAlive())
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