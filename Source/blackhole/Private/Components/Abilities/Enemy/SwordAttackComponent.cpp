#include "Components/Abilities/Enemy/SwordAttackComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/DamageEvents.h"

USwordAttackComponent::USwordAttackComponent()
{
	Cooldown = 1.5f; // Slightly slower than stab
	Range = 180.0f;
	WPCost = 0.0f; // Basic attack has no cost
	bIsBasicAbility = true; // This is a basic ability
}

void USwordAttackComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Set range from component
	Range = AttackRange;
	
	// Adjust cooldown based on attack speed
	if (AttackSpeed > 0.0f)
	{
		Cooldown = 1.5f / AttackSpeed;
	}
}

void USwordAttackComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute();
	PerformSwordAttack();
}

void USwordAttackComponent::PerformSwordAttack()
{
	if (!GetOwner()) return;
	
	AActor* Owner = GetOwner();
	FVector StartLocation = Owner->GetActorLocation();
	FVector ForwardVector = Owner->GetActorForwardVector();
	
	// Perform a cone check for targets
	TArray<FHitResult> HitResults;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(AttackRange);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	
	// Sweep in front of the enemy
	FVector EndLocation = StartLocation + (ForwardVector * AttackRange);
	
	if (GetWorld()->SweepMultiByChannel(HitResults, StartLocation, EndLocation, FQuat::Identity, 
		ECC_Pawn, CollisionShape, QueryParams))
	{
		for (const FHitResult& Hit : HitResults)
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				// Check if target is within attack cone
				FVector ToTarget = (HitActor->GetActorLocation() - StartLocation).GetSafeNormal();
				float DotProduct = FVector::DotProduct(ForwardVector, ToTarget);
				float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
				
				if (AngleToTarget <= AttackAngle)
				{
					// Apply damage
					FPointDamageEvent DamageEvent(BaseDamage, Hit, ForwardVector, nullptr);
					HitActor->TakeDamage(BaseDamage, DamageEvent, nullptr, Owner);
					
					UE_LOG(LogTemp, Warning, TEXT("SwordAttack: Dealt %.0f damage to %s"), BaseDamage, *HitActor->GetName());
				}
			}
		}
	}
	
	#if WITH_EDITOR
	// Debug visualization
	DrawDebugCone(GetWorld(), StartLocation, ForwardVector, AttackRange, 
		FMath::DegreesToRadians(AttackAngle), FMath::DegreesToRadians(AttackAngle), 
		12, FColor::Yellow, false, 1.0f);
	#endif
}