#include "Enemy/CombatEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/Abilities/Enemy/DodgeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Enemy/EnemyUtility.h"
#include "Enemy/AI/CombatEnemyStateMachine.h"

ACombatEnemy::ACombatEnemy()
{
	// Replace base state machine with combat-specific one
	StateMachine = CreateDefaultSubobject<UCombatEnemyStateMachine>(TEXT("CombatStateMachine"));
	
	SmashAbility = CreateDefaultSubobject<USmashAbilityComponent>(TEXT("SmashAbility"));
	BlockAbility = CreateDefaultSubobject<UBlockComponent>(TEXT("BlockAbility"));
	DodgeAbility = CreateDefaultSubobject<UDodgeComponent>(TEXT("DodgeAbility"));

	AttackRange = 200.0f; // Increased attack range
	ChaseRange = 2000.0f; // More persistent
	DodgeChance = 0.25f; // 25% chance to dodge
	BlockChance = 0.25f; // 25% chance to block
	
	// Configure combat enemy movement - balanced
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 400.0f; // Medium speed
		Movement->MaxAcceleration = 600.0f; // Medium acceleration
		Movement->BrakingDecelerationWalking = 600.0f; // Medium stops
		Movement->RotationRate = FRotator(0.0f, 360.0f, 0.0f); // Normal turning
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
	}
	
	// Set default data table row name
	StatsRowName = FName("Combat");
	
	// Create shield mesh component
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
	ShieldMesh->SetupAttachment(GetMesh(), FName("shieldsocket"));
	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMesh->SetCastShadow(true);
	ShieldMesh->SetVisibility(false); // Initially hidden
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
	// Call parent implementation first to handle combat detection
	Super::UpdateAIBehavior(DeltaTime);
	
	if (!TargetActor || !IntegrityComponent->IsAlive())
	{
		return;
	}

	float Distance = UEnemyUtility::GetDistanceToTarget(this, TargetActor);

	if (Distance <= AttackRange)
	{
		TryAttack();
	}
	else if (Distance <= ChaseRange)
	{
		UEnemyUtility::MoveTowardsTarget(this, TargetActor, DeltaTime, 300.0f);
	}
}

void ACombatEnemy::MoveTowardsTarget(float DeltaTime)
{
	// Now handled by UEnemyUtility::MoveTowardsTarget
	UEnemyUtility::MoveTowardsTarget(this, TargetActor, DeltaTime, 300.0f);
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
	return UEnemyUtility::GetDistanceToTarget(this, TargetActor);
}