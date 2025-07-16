#include "Components/Abilities/Enemy/HeatAuraComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Components/Attributes/WillPowerComponent.h"

UHeatAuraComponent::UHeatAuraComponent()
{
	Cooldown = 0.0f; // No cooldown, it's a passive ability
	Range = 300.0f;
	WPCost = 0.0f; // Passive ability has no cost
	bIsBasicAbility = false;
	bAuraActive = false;
}

void UHeatAuraComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Set range from component
	Range = AuraRadius;
	
	// Auto-activate on begin play
	Execute();
}

void UHeatAuraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up timer
	if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(AuraTickHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(AuraTickHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

void UHeatAuraComponent::Execute()
{
	if (bAuraActive)
	{
		return;
	}
	
	Super::Execute();
	SetAuraActive(true);
}

void UHeatAuraComponent::Deactivate()
{
	SetAuraActive(false);
	Super::Deactivate();
}

void UHeatAuraComponent::SetAuraActive(bool bActive)
{
	bAuraActive = bActive;
	
	if (bActive)
	{
		// Start the aura tick
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(AuraTickHandle, this, &UHeatAuraComponent::AuraTick, TickInterval, true);
			// Apply damage immediately
			AuraTick();
		}
	}
	else
	{
		// Stop the aura tick
		if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(AuraTickHandle))
		{
			GetWorld()->GetTimerManager().ClearTimer(AuraTickHandle);
		}
	}
}

void UHeatAuraComponent::AuraTick()
{
	if (!bAuraActive || !GetOwner())
	{
		return;
	}
	
	ApplyAuraDamage();
}

void UHeatAuraComponent::ApplyAuraDamage()
{
	if (!GetOwner()) return;
	
	AActor* Owner = GetOwner();
	FVector OwnerLocation = Owner->GetActorLocation();
	
	// Find all actors in radius
	TArray<AActor*> ActorsInRange;
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), OwnerLocation, AuraRadius, 
		TArray<TEnumAsByte<EObjectTypeQuery>>(), nullptr, TArray<AActor*>{Owner}, ActorsInRange);
	
	for (AActor* Actor : ActorsInRange)
	{
		// Check if it's a player
		if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(Actor))
		{
			// Drain WP from player
			if (UWillPowerComponent* WPComp = Player->FindComponentByClass<UWillPowerComponent>())
			{
				WPComp->DrainWillPower(DamagePerSecond * TickInterval);
				UE_LOG(LogTemp, Warning, TEXT("HeatAura: Draining %.0f WP from player"), DamagePerSecond * TickInterval);
			}
		}
		// Check if it's another enemy and we affect enemies
		else if (bAffectsEnemies && Actor->IsA<ABaseEnemy>() && Actor != Owner)
		{
			// Drain WP from enemy
			if (UWillPowerComponent* WPComp = Actor->FindComponentByClass<UWillPowerComponent>())
			{
				WPComp->DrainWillPower(DamagePerSecond * TickInterval);
			}
		}
	}
	
	#if WITH_EDITOR
	// Debug visualization
	DrawDebugSphere(GetWorld(), OwnerLocation, AuraRadius, 16, FColor::Orange, false, TickInterval - 0.1f);
	#endif
}