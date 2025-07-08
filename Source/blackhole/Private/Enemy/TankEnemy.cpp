#include "Enemy/TankEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

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

	float Distance = GetDistanceToTarget();

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
		MoveTowardsTarget(DeltaTime);
	}
}

void ATankEnemy::MoveTowardsTarget(float DeltaTime)
{
	if (!TargetActor || BlockAbility->IsBlocking())
	{
		return; // Don't move while blocking
	}

	FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	AddMovementInput(Direction, 0.8f); // Move slower than other enemies

	FRotator NewRotation = Direction.Rotation();
	NewRotation.Pitch = 0.0f;
	NewRotation.Roll = 0.0f;
	SetActorRotation(NewRotation);
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
	if (!TargetActor)
	{
		return FLT_MAX;
	}

	return FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
}

bool ATankEnemy::IsPlayerAttacking() const
{
	// Simple check - in a real game you'd check if player's attack animation is playing
	// or if an attack ability was recently used
	return GetDistanceToTarget() < 250.0f;
}