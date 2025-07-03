#include "BaseEnemy.h"
#include "../Components/Attributes/IntegrityComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABaseEnemy::ABaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	IntegrityComponent = CreateDefaultSubobject<UIntegrityComponent>(TEXT("Integrity"));
	
	// Tag all enemies so hack abilities can identify them
	Tags.Add(FName("Enemy"));
	
	bIsDead = false;
}

void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bIsDead)
	{
		// Check if dead
		if (IntegrityComponent && IntegrityComponent->GetCurrentValue() <= 0.0f)
		{
			OnDeath();
		}
		else
		{
			UpdateAIBehavior(DeltaTime);
		}
	}
}

void ABaseEnemy::UpdateAIBehavior(float DeltaTime)
{
}

AActor* ABaseEnemy::GetTargetActor() const
{
	return TargetActor;
}

void ABaseEnemy::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void ABaseEnemy::OnDeath()
{
	if (bIsDead)
	{
		return;
	}
	
	bIsDead = true;
	
	// Stop all movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	
	// Disable collision on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Enable ragdoll physics
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	
	// Add some impulse to make the death more dramatic
	FVector ImpulseDirection = GetActorLocation() - (TargetActor ? TargetActor->GetActorLocation() : GetActorLocation());
	ImpulseDirection.Normalize();
	ImpulseDirection.Z = 0.5f; // Add some upward force
	GetMesh()->AddImpulse(ImpulseDirection * 5000.0f, NAME_None, true);
	
	// Destroy actor after delay
	SetLifeSpan(10.0f);
}