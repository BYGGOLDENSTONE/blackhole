#include "Enemy/TankEnemy.h"
#include "Components/Abilities/Enemy/AreaDamageAbilityComponent.h"
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
	AreaDamageAbility = CreateDefaultSubobject<UAreaDamageAbilityComponent>(TEXT("AreaDamageAbility"));
	BlockAbility = CreateDefaultSubobject<UBlockComponent>(TEXT("BlockAbility"));
	// NO DodgeAbility - this enemy cannot dodge!

	AttackRange = 200.0f; // Increased attack range
	ChaseRange = 1500.0f; // More persistent
	BlockChance = 0.5f; // 50% chance to block when player is attacking
	
	// Configure area damage ability for ground slam
	if (AreaDamageAbility)
	{
		// Set default ground slam configuration
		AreaDamageAbility->DamagePattern = EAreaDamagePattern::Circular;
		AreaDamageAbility->DamageRadius = 800.0f;
		AreaDamageAbility->BaseDamage = 30.0f;
		AreaDamageAbility->bApplyKnockback = true;
		AreaDamageAbility->KnockbackForce = 750.0f;
		AreaDamageAbility->bApplyStagger = true;
		AreaDamageAbility->StaggerDuration = 1.5f;
		AreaDamageAbility->CustomAbilityName = "Ground Slam";
		AreaDamageAbility->SetCooldown(5.0f);
		AreaDamageAbility->PreDamageDelay = 0.5f;
	}

	// Configure tank movement settings - heavy and slow
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 300.0f; // Very slow
		Movement->MaxAcceleration = 400.0f; // Slow acceleration
		Movement->BrakingDecelerationWalking = 400.0f; // Slow stops
		Movement->RotationRate = FRotator(0.0f, 120.0f, 0.0f); // Slow turning
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
		
		// Tank is heavy - less affected by impulses
		Movement->Mass = 200.0f; // Double the default mass
	}
	
	// Set default data table row name
	StatsRowName = FName("Tank");
	
	// Tank enemies give more WP on kill due to higher difficulty
	WPRewardOnKill = 20.0f;
	
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

	// Tank has more health (as WP)
	MaxWP = 150.0f;
	CurrentWP = MaxWP;
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
	if (AreaDamageAbility && AreaDamageAbility->CanExecute())
	{
		AreaDamageAbility->Execute();
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

// Removed GetDamage and SetDamage - now handled by AreaDamageAbilityComponent properties