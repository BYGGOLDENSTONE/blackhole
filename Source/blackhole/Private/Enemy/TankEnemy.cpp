#include "Enemy/TankEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Enemy/EnemyUtility.h"
#include "Enemy/AI/TankEnemyStateMachine.h"

ATankEnemy::ATankEnemy()
{
	// Replace base state machine with tank-specific one
	StateMachine = CreateDefaultSubobject<UTankEnemyStateMachine>(TEXT("TankStateMachine"));
	
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
	// State machine handles all AI behavior now
	// This method only exists for compatibility
	Super::UpdateAIBehavior(DeltaTime);
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

void ATankEnemy::SetShieldVisible(bool bVisible)
{
	if (ShieldMesh)
	{
		ShieldMesh->SetVisibility(bVisible);
	}
}

float ATankEnemy::GetDamage() const
{
	if (SmashAbility)
	{
		return SmashAbility->GetDamage();
	}
	return 20.0f; // Default damage
}

void ATankEnemy::SetDamage(float NewDamage)
{
	if (SmashAbility)
	{
		SmashAbility->SetDamage(NewDamage);
	}
}