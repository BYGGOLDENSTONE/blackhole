#include "Components/Abilities/Player/Utility/ForgeDashAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Enemy/BaseEnemy.h"

UForgeDashAbility::UForgeDashAbility()
{
	// Set forge path
	PathType = ECharacterPath::Forge;
	
	// Mark as basic ability - not affected by ultimate system
	bIsBasicAbility = true;
	
	// Override base parameters
	Cost = 5.0f; // Legacy field
	StaminaCost = 5.0f; // New dual resource system
	WPCost = 0.0f; // Utility abilities don't consume WP
	HeatCost = 0.0f; // New dual resource system
	Cooldown = 2.0f;
	MovementDuration = ChargeDuration;
	
	// Forge dash specifics
	ChargeSpeed = 2000.0f;
	ChargeDuration = 0.3f;
	ImpactDamage = 10.0f;
	ImpactRadius = 150.0f;
	StaggerDuration = 0.5f;
}

void UForgeDashAbility::ApplyMovement(ACharacter* Character)
{
	if (!Character || !CachedMovement)
	{
		return;
	}
	
	// Get charge direction (always forward for forge dash)
	FVector ChargeDirection = Character->GetActorForwardVector();
	ChargeDirection.Z = 0.0f; // Keep horizontal
	ChargeDirection.Normalize();
	
	// Store original friction
	OriginalFriction = CachedMovement->BrakingFrictionFactor;
	
	// Reduce friction for charge
	CachedMovement->BrakingFrictionFactor = 0.1f;
	
	// Set charging state
	bIsCharging = true;
	
	// Apply charge velocity
	CachedMovement->Velocity = ChargeDirection * ChargeSpeed;
	
	// Small upward velocity to clear obstacles
	if (CachedMovement->IsMovingOnGround())
	{
		CachedMovement->Velocity.Z = 100.0f;
	}
	
	// Bind hit event
	if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
	{
		Capsule->OnComponentHit.AddDynamic(this, &UForgeDashAbility::OnCharacterHit);
	}
	
	#if WITH_EDITOR
	// Debug visualization
	if (UWorld* World = GetWorld())
	{
		FVector Start = Character->GetActorLocation();
		FVector End = Start + (ChargeDirection * ChargeSpeed * ChargeDuration);
		DrawDebugLine(World, Start, End, FColor::Orange, false, 2.0f, 0, 8.0f);
		DrawDebugSphere(World, End, ImpactRadius, 12, FColor::Red, false, 2.0f);
	}
	#endif
}

void UForgeDashAbility::StopMovement()
{
	if (ACharacter* Character = GetCharacterOwner())
	{
		// Unbind hit event
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Capsule->OnComponentHit.RemoveDynamic(this, &UForgeDashAbility::OnCharacterHit);
		}
		
		// Check for impact damage at end of charge
		if (bIsCharging)
		{
			CheckImpactDamage();
		}
	}
	
	if (CachedMovement)
	{
		// Restore friction
		CachedMovement->BrakingFrictionFactor = OriginalFriction;
		
		// Apply heavy braking
		CachedMovement->Velocity *= 0.2f;
	}
	
	bIsCharging = false;
}

void UForgeDashAbility::OnCharacterHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AActor* Owner = GetOwner();
	if (!bIsCharging || !OtherActor || !Owner || OtherActor == Owner)
	{
		return;
	}
	
	// Check if we hit an enemy
	if (OtherActor->ActorHasTag("Enemy") || OtherActor->IsA<ABaseEnemy>())
	{
		// Apply damage
		if (UIntegrityComponent* TargetIntegrity = OtherActor->FindComponentByClass<UIntegrityComponent>())
		{
			TargetIntegrity->TakeDamage(ImpactDamage);
		}
		
		// Apply stagger (knockback)
		if (ACharacter* EnemyCharacter = Cast<ACharacter>(OtherActor))
		{
			FVector KnockbackDirection = (OtherActor->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
			KnockbackDirection.Z = 0.3f; // Add slight upward component
			
			if (UCharacterMovementComponent* EnemyMovement = EnemyCharacter->GetCharacterMovement())
			{
				EnemyMovement->AddImpulse(KnockbackDirection * 500.0f, true);
			}
		}
		
		// End charge early on impact
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(MovementTimerHandle);
		}
		StopMovement();
	}
}

void UForgeDashAbility::CheckImpactDamage()
{
	ACharacter* Character = GetCharacterOwner();
	if (!Character)
	{
		return;
	}
	
	// Get all actors in impact radius
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	
	FVector CharacterLocation = Character->GetActorLocation();
	
	GetWorld()->SweepMultiByChannel(
		HitResults,
		CharacterLocation,
		CharacterLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(ImpactRadius),
		QueryParams
	);
	
	// Apply damage and stagger to all enemies in radius
	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || (!HitActor->ActorHasTag("Enemy") && !HitActor->IsA<ABaseEnemy>()))
		{
			continue;
		}
		
		// Apply damage
		if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
		{
			TargetIntegrity->TakeDamage(ImpactDamage * 0.5f); // Half damage for area effect
		}
		
		// Apply knockback
		if (ACharacter* EnemyCharacter = Cast<ACharacter>(HitActor))
		{
			FVector KnockbackDirection = (HitActor->GetActorLocation() - CharacterLocation).GetSafeNormal();
			KnockbackDirection.Z = 0.3f;
			
			if (UCharacterMovementComponent* EnemyMovement = EnemyCharacter->GetCharacterMovement())
			{
				EnemyMovement->AddImpulse(KnockbackDirection * 300.0f, true);
			}
		}
	}
	
	#if WITH_EDITOR
	// Debug visualization of impact
	DrawDebugSphere(GetWorld(), CharacterLocation, ImpactRadius, 16, FColor::Yellow, false, 1.0f);
	#endif
}