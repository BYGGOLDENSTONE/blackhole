#include "Enemy/TankEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Enemy/EnemyUtility.h"

ATankEnemy::ATankEnemy()
{
	// Only create the abilities this enemy type should have
	SmashAbility = CreateDefaultSubobject<USmashAbilityComponent>(TEXT("SmashAbility"));
	BlockAbility = CreateDefaultSubobject<UBlockComponent>(TEXT("BlockAbility"));
	// NO DodgeAbility - this enemy cannot dodge!

	AttackRange = 150.0f;
	ChaseRange = 800.0f; // Shorter chase range - tank is slower
	BlockChance = 0.5f; // 50% chance to block when player is attacking

	// Make tank enemy slower
	GetCharacterMovement()->MaxWalkSpeed = 400.0f; // Default is usually 600
	
	// Create shield mesh component
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
	ShieldMesh->SetupAttachment(GetMesh(), FName("shieldsocket"));
	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMesh->SetCastShadow(true);
	ShieldMesh->SetVisibility(false); // Initially hidden
}

void ATankEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}

	// Tank has more health
	if (IntegrityComponent)
	{
		IntegrityComponent->SetMaxValue(150.0f);
		IntegrityComponent->SetCurrentValue(150.0f);
	}
}

void ATankEnemy::UpdateAIBehavior(float DeltaTime)
{
	// Call parent implementation first to handle combat detection
	Super::UpdateAIBehavior(DeltaTime);
	
	if (!TargetActor || !IntegrityComponent->IsAlive())
	{
		return;
	}

	float Distance = UEnemyUtility::GetDistanceToTarget(this, TargetActor);

	// When player is attacking and close, try to block
	if (Distance <= 300.0f && IsPlayerAttacking() && FMath::FRand() < BlockChance)
	{
		TryBlock();
	}

	if (Distance <= AttackRange && !BlockAbility->IsBlocking())
	{
		TryAttack();
	}
	else if (Distance <= ChaseRange)
	{
		// Use utility function for movement
		UEnemyUtility::MoveTowardsTarget(this, TargetActor, DeltaTime, 240.0f); // Tank moves slower
	}
}

void ATankEnemy::MoveTowardsTarget(float DeltaTime)
{
	// Now handled by UEnemyUtility::MoveTowardsTarget
	if (!BlockAbility->IsBlocking())
	{
		UEnemyUtility::MoveTowardsTarget(this, TargetActor, DeltaTime, 240.0f);
	}
}

void ATankEnemy::TryAttack()
{
	if (SmashAbility && SmashAbility->CanExecute())
	{
		SmashAbility->Execute();
	}
}

void ATankEnemy::TryBlock()
{
	if (BlockAbility && BlockAbility->CanExecute())
	{
		BlockAbility->Execute();
	}
}

float ATankEnemy::GetDistanceToTarget() const
{
	return UEnemyUtility::GetDistanceToTarget(this, TargetActor);
}

bool ATankEnemy::IsPlayerAttacking() const
{
	// Simple proximity check - in a real game you'd check if player's attack animation is playing
	return UEnemyUtility::GetDistanceToTarget(this, TargetActor) < 250.0f;
}