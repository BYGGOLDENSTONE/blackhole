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
	WPCost = 20.0f; // New dual resource system
	// HeatCost removed - heat system no longer exists
	Cooldown = 4.0f;
	Range = 1500.0f;
	ArmorReductionPercent = 0.5f;
	EffectDuration = 5.0f;
	
	// Ensure this is NOT a basic ability
	bIsBasicAbility = false;
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
	
	// IMPORTANT: If we're in ultimate mode, the base class already called ExecuteUltimate()
	// We should not continue with normal execution
	if (IsInUltimateMode())
	{
		return;
	}
	
	// Normal ability execution continues below
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
		
		if (UWorld* World = GetWorld())
		{
			if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
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
						if (UWorld* TimerWorld = GetWorld())
						{
							TimerWorld->GetTimerManager().SetTimer(RemoveEffectTimer, 
								[HitActor]()
								{
									if (IsValid(HitActor))
									{
										HitActor->Tags.Remove("ArmorBreached");
									}
								}, 
								EffectDuration, false);
						}
						
						UE_LOG(LogTemp, Log, TEXT("FirewallBreach: Applied armor shred to %s"), *HitActor->GetName());
					}
				}
			}
			
			#if WITH_EDITOR
			DrawDebugLine(World, Start, End, FColor::Purple, false, 1.0f, 0, 2.0f);
			#endif
		}
	}
}

void UFirewallBreachAbility::ExecuteUltimate()
{
	// Ultimate Firewall Breach - "Total System Compromise"
	// Instantly removes ALL armor from ALL enemies on screen and makes them vulnerable
	UE_LOG(LogTemp, Warning, TEXT("ULTIMATE FIREWALL BREACH: Total System Compromise!"));
	
	Super::ExecuteUltimate();
	
	if (AActor* Owner = GetOwner())
	{
		// Ultimate version: Affect ALL enemies in massive radius
		float UltimateRadius = Range * UltimateRadiusMultiplier;
		// Use editable ultimate properties instead of hardcoded values
		
		// Find all enemies in radius
		TArray<FHitResult> HitResults;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Owner);
		
		FVector OwnerLocation = Owner->GetActorLocation();
		
		if (UWorld* World = GetWorld())
		{
			World->SweepMultiByChannel(
				HitResults,
				OwnerLocation,
				OwnerLocation,
				FQuat::Identity,
				ECC_Pawn,
				FCollisionShape::MakeSphere(UltimateRadius),
				QueryParams
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("FirewallBreachAbility: Cannot sweep - no valid world"));
			return;
		}
		
		int32 EnemiesBreached = 0;
		
		// Apply total armor breach to all enemies
		for (const FHitResult& Result : HitResults)
		{
			if (AActor* HitActor = Result.GetActor())
			{
				if (HitActor->ActorHasTag("Enemy") || HitActor->IsA<ABaseEnemy>())
				{
					// Apply total armor breach
					HitActor->Tags.AddUnique("TotalArmorBreach");
					HitActor->Tags.AddUnique("ArmorBreached");
					
					// Visual effect - make enemy glow or show vulnerability
					// This would be implemented with material changes in Blueprint
					
					// Set timer to remove effect
					FTimerHandle RemoveEffectTimer;
					if (UWorld* TimerWorld = GetWorld())
					{
						TimerWorld->GetTimerManager().SetTimer(RemoveEffectTimer, 
							[HitActor]()
							{
								if (IsValid(HitActor))
								{
									HitActor->Tags.Remove("TotalArmorBreach");
									HitActor->Tags.Remove("ArmorBreached");
								}
							}, 
							UltimateDuration, false);
					}
					
					EnemiesBreached++;
					
					UE_LOG(LogTemp, Warning, TEXT("Total System Compromise: Breached %s - 100%% vulnerable!"), 
						*HitActor->GetName());
				}
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Ultimate Firewall Breach: Compromised %d enemy systems!"), EnemiesBreached);
		
		#if WITH_EDITOR
		// Draw debug sphere showing affected area
		if (UWorld* World = GetWorld())
		{
			DrawDebugSphere(World, OwnerLocation, UltimateRadius, 32, FColor::Purple, false, 2.0f);
			
			// Draw lines to all breached enemies
			for (const FHitResult& Result : HitResults)
			{
				if (AActor* HitActor = Result.GetActor())
				{
					if (HitActor->ActorHasTag("TotalArmorBreach"))
					{
						DrawDebugLine(World, OwnerLocation, HitActor->GetActorLocation(), 
							FColor::Purple, false, 2.0f, 0, 3.0f);
					}
				}
			}
		}
		#endif
	}
}