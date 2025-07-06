#include "Components/Abilities/Player/Hacker/FirewallBreachAbility.h"
#include "Systems/ResourceManager.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"

UFirewallBreachAbility::UFirewallBreachAbility()
{
	Cost = 20.0f; // Legacy field
	StaminaCost = 15.0f; // New dual resource system
	WPCost = 20.0f; // New dual resource system
	HeatCost = 0.0f; // Hacker abilities don't consume heat
	Cooldown = 4.0f;
	Range = 1500.0f;
	ArmorReductionPercent = 0.5f;
	EffectDuration = 5.0f;
}

void UFirewallBreachAbility::BeginPlay()
{
	Super::BeginPlay();
}

bool UFirewallBreachAbility::CanExecute() const
{
	// Let base class handle all resource checks
	return Super::CanExecute();
}

void UFirewallBreachAbility::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	// Call base class to handle resource costs and cooldown
	Super::Execute();
	
	if (AActor* Owner = GetOwner())
	{
		FVector Start;
		FVector Forward;
		
		// Use camera for aiming
		if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Owner))
		{
			if (UCameraComponent* Camera = PlayerOwner->GetCameraComponent())
			{
				Start = Camera->GetComponentLocation();
				Forward = Camera->GetForwardVector();
			}
			else
			{
				Start = Owner->GetActorLocation();
				Forward = Owner->GetActorForwardVector();
			}
		}
		else
		{
			Start = Owner->GetActorLocation();
			Forward = Owner->GetActorForwardVector();
		}
		
		FVector End = Start + (Forward * Range);
		
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Owner);
		
		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
		{
			if (AActor* HitActor = HitResult.GetActor())
			{
				// Apply armor shred effect to enemy
				if (HitActor->ActorHasTag("Enemy") || HitActor->IsA<ABaseEnemy>())
				{
					// Simple implementation: Tag the enemy as armor-breached
					HitActor->Tags.AddUnique("ArmorBreached");
					
					// Set timer to remove effect
					FTimerHandle RemoveEffectTimer;
					GetWorld()->GetTimerManager().SetTimer(RemoveEffectTimer, 
						[HitActor]()
						{
							if (IsValid(HitActor))
							{
								HitActor->Tags.Remove("ArmorBreached");
							}
						}, 
						EffectDuration, false);
					
					UE_LOG(LogTemp, Log, TEXT("FirewallBreach: Applied armor shred to %s"), *HitActor->GetName());
				}
			}
		}
		
		#if WITH_EDITOR
		DrawDebugLine(GetWorld(), Start, End, FColor::Purple, false, 1.0f, 0, 2.0f);
		#endif
	}
}