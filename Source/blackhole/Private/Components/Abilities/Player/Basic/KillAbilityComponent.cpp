#include "Components/Abilities/Player/Basic/KillAbilityComponent.h"
#include "Systems/ResourceManager.h"
#include "GameFramework/Actor.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"

UKillAbilityComponent::UKillAbilityComponent()
{
	// Debug ability - no resource costs
	WPCost = 0.0f;
	Cooldown = 5.0f;
	Range = 3000.0f;
	
	// Kill is not a basic ability - it can have ultimate version
	bIsBasicAbility = false;
}

void UKillAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UKillAbilityComponent::CanExecute() const
{
	// Debug ability - just check base conditions
	return Super::CanExecute();
}

void UKillAbilityComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute(); // Handle cooldown and logging
	
	if (AActor* Owner = GetOwner())
	{
		FVector Start;
		FVector End;
		
		// Use camera for aiming - trace directly from camera through crosshair
		if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Owner))
		{
			if (UCameraComponent* Camera = PlayerOwner->GetCameraComponent())
			{
				// IMPORTANT: Trace directly from camera for consistent aiming
				Start = Camera->GetComponentLocation();
				FVector CameraForward = Camera->GetForwardVector();
				End = Start + (CameraForward * Range);
			}
			else
			{
				// Fallback - use character position
				Start = Owner->GetActorLocation();
				End = Start + (Owner->GetActorForwardVector() * Range);
			}
		}
		else
		{
			// Non-player owners use their actor location
			Start = Owner->GetActorLocation();
			End = Start + (Owner->GetActorForwardVector() * Range);
		}
		
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Owner);
		
		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
		{
			if (AActor* HitActor = HitResult.GetActor())
			{
				// Check if the target is an enemy (has tag "Enemy" or is ABaseEnemy)
				if (HitActor->ActorHasTag("Enemy") || HitActor->IsA<ABaseEnemy>())
				{
					// Instant kill - deal massive damage
					float KillDamage = 99999.0f;
					
					// Use actor's TakeDamage (routes to WP)
					FVector ImpactDirection = (HitActor->GetActorLocation() - Start).GetSafeNormal();
					FPointDamageEvent DamageEvent(KillDamage, HitResult, ImpactDirection, nullptr);
					HitActor->TakeDamage(KillDamage, DamageEvent, nullptr, Owner);
				}
			}
		}
		
		#if WITH_EDITOR
		// Only draw debug line for non-player owners (enemies)
		if (!Owner->IsA<ABlackholePlayerCharacter>())
		{
			DrawDebugLine(GetWorld(), Start, End, FColor::Black, false, 1.0f, 0, 3.0f);
		}
		#endif
	}
}

void UKillAbilityComponent::ExecuteUltimate()
{
	// Ultimate Kill - kills all enemies on screen
	UE_LOG(LogTemp, Warning, TEXT("ULTIMATE KILL: Death Wave!"));
	
	Super::ExecuteUltimate();
	
	// Debug message removed - ultimate kill
	
	// Find all enemies in a large radius
	if (AActor* Owner = GetOwner())
	{
		TArray<FOverlapResult> OverlapResults;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Owner);
		
		// Massive radius for ultimate
		float UltimateRadius = 5000.0f;
		
		FVector Location = Owner->GetActorLocation();
		GetWorld()->OverlapMultiByChannel(
			OverlapResults,
			Location,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(UltimateRadius),
			QueryParams
		);
		
		int32 KillCount = 0;
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (AActor* Target = Result.GetActor())
			{
				if (Target->ActorHasTag("Enemy") || Target->IsA<ABaseEnemy>())
				{
					// Instant kill - deal massive damage
					float KillDamage = 99999.0f;
					
					// Use actor's TakeDamage (routes to WP)
					FVector ImpactDirection = (Target->GetActorLocation() - Location).GetSafeNormal();
					FPointDamageEvent DamageEvent(KillDamage, FHitResult(), ImpactDirection, nullptr);
					Target->TakeDamage(KillDamage, DamageEvent, nullptr, Owner);
					
					KillCount++;
				}
			}
		}
		
		// Debug message removed - kill count
		
		#if WITH_EDITOR
		// Visual effect - expanding death ring
		for (int32 i = 0; i < 10; i++)
		{
			float Radius = (i / 10.0f) * UltimateRadius;
			DrawDebugSphere(GetWorld(), Location, Radius, 32, FColor::Red, false, 1.0f, 0, 2.0f);
		}
		#endif
	}
}