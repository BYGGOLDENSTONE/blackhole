#include "Components/Abilities/Enemy/ChargeAbilityComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Components/StatusEffectComponent.h"

UChargeAbilityComponent::UChargeAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Cooldown = 5.0f;
	Range = 1500.0f; // Max range to start charge
	WPCost = 0.0f; // No cost for enemy ability
	bIsBasicAbility = false;
	bIsCharging = false;
	ChargeTimeElapsed = 0.0f;
}

void UChargeAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false); // Only tick when charging
}

void UChargeAbilityComponent::Execute()
{
	if (!CanExecute() || bIsCharging)
	{
		return;
	}
	
	// Find target (usually the player)
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner) return;
	
	// Get the player
	ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player) return;
	
	// Check distance
	float DistanceToPlayer = FVector::Dist(Owner->GetActorLocation(), Player->GetActorLocation());
	if (DistanceToPlayer < MinChargeDistance || DistanceToPlayer > Range)
	{
		return;
	}
	
	ChargeTarget = Player;
	Super::Execute();
	StartCharge();
}

void UChargeAbilityComponent::StartCharge()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner || !ChargeTarget) return;
	
	bIsCharging = true;
	ChargeTimeElapsed = 0.0f;
	ChargeStartLocation = Owner->GetActorLocation();
	
	// Calculate charge direction
	FVector ToTarget = ChargeTarget->GetActorLocation() - ChargeStartLocation;
	ToTarget.Z = 0; // Keep charge horizontal
	ChargeDirection = ToTarget.GetSafeNormal();
	
	// Disable normal movement
	if (UCharacterMovementComponent* Movement = Owner->GetCharacterMovement())
	{
		Movement->SetMovementMode(EMovementMode::MOVE_Flying);
		Movement->StopMovementImmediately();
	}
	
	SetComponentTickEnabled(true);
	
	UE_LOG(LogTemp, Warning, TEXT("Tank starting charge towards target!"));
}

void UChargeAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsCharging)
	{
		UpdateCharge(DeltaTime);
	}
}

void UChargeAbilityComponent::UpdateCharge(float DeltaTime)
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner)
	{
		EndCharge(false);
		return;
	}
	
	ChargeTimeElapsed += DeltaTime;
	
	// Move the character
	FVector CurrentLocation = Owner->GetActorLocation();
	FVector NewLocation = CurrentLocation + (ChargeDirection * ChargeSpeed * DeltaTime);
	
	// Check for collision
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, CurrentLocation, NewLocation, ECC_Pawn, QueryParams))
	{
		// Hit something
		Owner->SetActorLocation(HitResult.Location);
		ApplyImpactDamage(HitResult.Location);
		EndCharge(true);
		return;
	}
	
	// Check max distance
	float DistanceTraveled = FVector::Dist(ChargeStartLocation, NewLocation);
	if (DistanceTraveled >= ChargeDistance)
	{
		EndCharge(false);
		return;
	}
	
	// Move to new location
	Owner->SetActorLocation(NewLocation);
	
	// Check if we're close enough to target for impact
	if (ChargeTarget && FVector::Dist(NewLocation, ChargeTarget->GetActorLocation()) < ImpactRadius)
	{
		ApplyImpactDamage(NewLocation);
		EndCharge(true);
	}
}

void UChargeAbilityComponent::EndCharge(bool bHitTarget)
{
	bIsCharging = false;
	SetComponentTickEnabled(false);
	
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (Owner && Owner->GetCharacterMovement())
	{
		Owner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Tank charge ended. Hit target: %s"), bHitTarget ? TEXT("Yes") : TEXT("No"));
}

void UChargeAbilityComponent::ApplyImpactDamage(const FVector& ImpactLocation)
{
	if (!GetOwner()) return;
	
	// Find all actors in impact radius
	TArray<AActor*> ActorsInRange;
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, ImpactRadius, 
		TArray<TEnumAsByte<EObjectTypeQuery>>(), nullptr, TArray<AActor*>{GetOwner()}, ActorsInRange);
	
	for (AActor* Actor : ActorsInRange)
	{
		// Calculate knockback direction
		FVector KnockbackDirection = (Actor->GetActorLocation() - ImpactLocation).GetSafeNormal();
		KnockbackDirection.Z = 0.3f; // Add some upward force
		KnockbackDirection.Normalize();
		
		// Apply damage
		FPointDamageEvent DamageEvent(ImpactDamage, FHitResult(), -KnockbackDirection, nullptr);
		Actor->TakeDamage(ImpactDamage, DamageEvent, nullptr, GetOwner());
		
		// Apply knockback
		if (ACharacter* Character = Cast<ACharacter>(Actor))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->Launch(KnockbackDirection * KnockbackForce);
			}
			
			// Apply knockdown effect
			if (UStatusEffectComponent* StatusEffect = Character->FindComponentByClass<UStatusEffectComponent>())
			{
				StatusEffect->ApplyStatusEffect(EStatusEffectType::Knockdown, 1.5f, 1.0f);
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Charge impact: Dealt %.0f damage and knockback to %s"), ImpactDamage, *Actor->GetName());
	}
	
	#if WITH_EDITOR
	// Debug visualization
	DrawDebugSphere(GetWorld(), ImpactLocation, ImpactRadius, 16, FColor::Red, false, 2.0f);
	#endif
}