#include "Components/Abilities/Player/Forge/HeatShieldAbility.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Player/BlackholePlayerCharacter.h"

UHeatShieldAbility::UHeatShieldAbility()
{
	// Ability costs and cooldown (per GDD)
	Cost = 20.0f; // Legacy field
	StaminaCost = 15.0f; // New dual resource system
	WPCost = 0.0f; // Forge abilities don't consume WP
	HeatCost = 20.0f; // New dual resource system
	Cooldown = 12.0f;
	HeatGenerationMultiplier = 0.6f;
	
	bShieldActive = false;
	CurrentShieldHealth = 0.0f;
}

bool UHeatShieldAbility::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	// Don't allow if shield is already active
	if (bShieldActive)
	{
		return false;
	}
	
	return true;
}

void UHeatShieldAbility::Execute()
{
	if (!CanExecute())
	{
		return;
	}

	Super::Execute();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	// Activate shield
	bShieldActive = true;
	CurrentShieldHealth = MaxShieldHealth;

	// Register with IntegrityComponent to intercept damage
	if (UIntegrityComponent* IntegrityComp = Character->FindComponentByClass<UIntegrityComponent>())
	{
		// TODO: IntegrityComponent needs to check for shield before applying damage
		// This would require modifying IntegrityComponent to have a damage interception system
	}

	// Spawn shield visual effect
	if (ShieldEffect)
	{
		ActiveShieldEffect = UGameplayStatics::SpawnEmitterAttached(
			ShieldEffect,
			Character->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}

	// Play activation sound
	if (ShieldActivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldActivateSound, Character->GetActorLocation());
	}

	// Set timer for shield duration
	GetWorld()->GetTimerManager().SetTimer(
		ShieldDurationTimer,
		this,
		&UHeatShieldAbility::DeactivateShield,
		ShieldDuration,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("Heat Shield activated: %f HP for %f seconds"), MaxShieldHealth, ShieldDuration);
}

float UHeatShieldAbility::AbsorbDamage(float IncomingDamage)
{
	if (!bShieldActive || CurrentShieldHealth <= 0.0f)
	{
		return IncomingDamage;
	}

	float DamageToAbsorb = FMath::Min(IncomingDamage, CurrentShieldHealth);
	float RemainingDamage = IncomingDamage - DamageToAbsorb;

	// Reduce shield health
	CurrentShieldHealth -= DamageToAbsorb;

	// Broadcast damage absorbed
	OnShieldDamageAbsorbed.Broadcast(DamageToAbsorb);

	// Play hit sound
	if (ShieldHitSound && DamageToAbsorb > 0.0f)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldHitSound, GetOwner()->GetActorLocation());
	}

	// Check if shield broke
	if (CurrentShieldHealth <= 0.0f)
	{
		BreakShield(RemainingDamage);
	}

	// TODO: Implement damage reflection
	// Need to identify the damage source and reflect damage back

	UE_LOG(LogTemp, Log, TEXT("Shield absorbed %f damage, %f remaining shield health"), DamageToAbsorb, CurrentShieldHealth);

	return RemainingDamage;
}

void UHeatShieldAbility::DeactivateShield()
{
	if (!bShieldActive)
	{
		return;
	}

	bShieldActive = false;
	CurrentShieldHealth = 0.0f;

	// Stop visual effect
	if (ActiveShieldEffect)
	{
		ActiveShieldEffect->DeactivateSystem();
		ActiveShieldEffect->DestroyComponent();
		ActiveShieldEffect = nullptr;
	}

	// Clear timer
	GetWorld()->GetTimerManager().ClearTimer(ShieldDurationTimer);

	UE_LOG(LogTemp, Log, TEXT("Heat Shield deactivated"));
}

void UHeatShieldAbility::BreakShield(float RemainingDamage)
{
	// Broadcast shield broken event
	OnShieldBroken.Broadcast(RemainingDamage);

	// Play break effect
	if (ShieldBreakEffect && GetOwner())
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ShieldBreakEffect,
			GetOwner()->GetActorLocation(),
			FRotator::ZeroRotator,
			FVector(1.5f)
		);
	}

	// Play break sound
	if (ShieldBreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldBreakSound, GetOwner()->GetActorLocation());
	}

	// Deactivate shield
	DeactivateShield();

	UE_LOG(LogTemp, Warning, TEXT("Heat Shield broken! %f damage passed through"), RemainingDamage);
}

float UHeatShieldAbility::GetShieldHealthPercent() const
{
	return MaxShieldHealth > 0.0f ? CurrentShieldHealth / MaxShieldHealth : 0.0f;
}