#include "Components/Abilities/Enemy/SmashAbilityComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

USmashAbilityComponent::USmashAbilityComponent()
{
	Damage = 20.0f;  // Increased base damage
	Cooldown = 1.5f;
	Range = 250.0f;  // Increased range
	bIsAreaDamage = false;  // Default to single target
	AreaRadius = 300.0f;    // Area damage radius
	KnockbackForce = 750.0f; // Default knockback force
}

void USmashAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USmashAbilityComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute();
	
	if (AActor* Owner = GetOwner())
	{
		if (bIsAreaDamage)
		{
			// Area damage for ground slam
			PerformAreaDamage(Owner);
		}
		else
		{
			// Single target damage
			PerformSingleTargetDamage(Owner);
		}
	}
}

void USmashAbilityComponent::PerformSingleTargetDamage(AActor* Owner)
{
	FVector Start = Owner->GetActorLocation();
	FVector Forward = Owner->GetActorForwardVector();
	FVector End = Start + (Forward * Range);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			// Apply damage using actor's TakeDamage method (routes to WP)
			FPointDamageEvent DamageEvent(Damage, HitResult, Owner->GetActorForwardVector(), nullptr);
			HitActor->TakeDamage(Damage, DamageEvent, nullptr, Owner);
			UE_LOG(LogTemp, Warning, TEXT("SmashAbility: Dealt %f damage to %s"), Damage, *HitActor->GetName());
		}
	}
	
	#if WITH_EDITOR
	DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 2.0f);
	#endif
}

void USmashAbilityComponent::PerformAreaDamage(AActor* Owner)
{
	FVector Center = Owner->GetActorLocation();
	
	// Find all actors in radius
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(AreaRadius);
	
	if (GetWorld()->OverlapMultiByChannel(OverlapResults, Center, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams))
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (AActor* HitActor = Result.GetActor())
			{
				// Skip damaging self
				if (HitActor == Owner) continue;
				
				// Damage falloff based on distance
				float Distance = FVector::Dist(Center, HitActor->GetActorLocation());
				float DamageFalloff = FMath::Clamp(1.0f - (Distance / AreaRadius), 0.3f, 1.0f);
				float FinalDamage = Damage * DamageFalloff;
				
				// Apply damage using actor's TakeDamage method (routes to WP)
				FVector ImpactDirection = (HitActor->GetActorLocation() - Center).GetSafeNormal();
				FPointDamageEvent DamageEvent(FinalDamage, FHitResult(), ImpactDirection, nullptr);
				HitActor->TakeDamage(FinalDamage, DamageEvent, nullptr, Owner);
				
				// Apply knockback to all characters (players and enemies)
				if (ACharacter* TargetCharacter = Cast<ACharacter>(HitActor))
				{
					// Calculate knockback direction
					FVector KnockbackDirection = (HitActor->GetActorLocation() - Center).GetSafeNormal();
					KnockbackDirection.Z = 0.3f; // Add slight upward force
					KnockbackDirection.Normalize();
					
					// Calculate knockback force based on distance (closer = stronger)
					float ActualKnockbackForce = KnockbackForce * DamageFalloff;
					
					if (UCharacterMovementComponent* CharMovement = TargetCharacter->GetCharacterMovement())
					{
						// Launch the character
						CharMovement->Launch(KnockbackDirection * ActualKnockbackForce);
						UE_LOG(LogTemp, Warning, TEXT("SmashAbility Area: Applied knockback force %f to %s"), ActualKnockbackForce, *HitActor->GetName());
					}
				}
				
				UE_LOG(LogTemp, Warning, TEXT("SmashAbility Area: Dealt %f damage to %s"), FinalDamage, *HitActor->GetName());
			}
		}
	}
	
	#if WITH_EDITOR
	DrawDebugSphere(GetWorld(), Center, AreaRadius, 12, FColor::Red, false, 1.0f);
	#endif
}