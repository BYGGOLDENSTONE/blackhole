#include "Components/Abilities/Player/Basic/SlashAbilityComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "WorldCollision.h"
#include "Systems/HitStopManager.h"
#include "GameFramework/PlayerController.h"

USlashAbilityComponent::USlashAbilityComponent()
{
	Damage = 20.0f;
	WPCost = 0.0f; // Slash is free - no WP cost
	Cooldown = 2.0f;
	Range = 200.0f;
	
	// Mark as basic ability - not affected by ultimate system
	bIsBasicAbility = true;
}

void USlashAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache owner character
	if (AActor* Owner = GetOwner())
	{
		OwnerCharacter = Cast<ABlackholePlayerCharacter>(Owner);
	}
}

bool USlashAbilityComponent::CanExecute() const
{
	// Let base class handle all resource checks
	return Super::CanExecute();
}

void USlashAbilityComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	// Execute normal slash
		// Base class handles resource costs (stamina + WP) and cooldown
		Super::Execute();
		
		// Combo registration removed - now handled by individual combo components
		
		// Normal slash logic
		if (AActor* Owner = GetOwner())
		{
			// Use camera for aiming if this is the player
			if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Owner))
			{
				if (UCameraComponent* Camera = PlayerOwner->GetCameraComponent())
				{
					// Step 1: Extended trace from camera (2x range beyond crosshair)
					FVector CameraLocation = Camera->GetComponentLocation();
					FVector CameraForward = Camera->GetForwardVector();
					float ExtendedRange = Range * 2.0f; // Extend trace to 2x distance
					FVector TraceEnd = CameraLocation + (CameraForward * ExtendedRange);
					
					FHitResult TraceHit;
					FCollisionQueryParams TraceParams;
					TraceParams.AddIgnoredActor(Owner);
					
					// Extended line trace
					bool bTraceHit = GetWorld()->LineTraceSingleByChannel(TraceHit, CameraLocation, TraceEnd, ECC_Pawn, TraceParams);
					
					// Step 2: Sphere check around player (300 unit radius)
					float SphereRadius = 300.0f; // Attack range around player
					FVector PlayerLocation = Owner->GetActorLocation();
					
					TArray<FOverlapResult> OverlapResults;
					FCollisionQueryParams SphereParams;
					SphereParams.AddIgnoredActor(Owner);
					
					bool bSphereHit = GetWorld()->OverlapMultiByChannel(
						OverlapResults,
						PlayerLocation,
						FQuat::Identity,
						ECC_Pawn,
						FCollisionShape::MakeSphere(SphereRadius),
						SphereParams
					);
					
					// Step 3: Check if BOTH trace and sphere hit the same target
					AActor* ValidTarget = nullptr;
					FHitResult ValidHit;
					
					if (bTraceHit && TraceHit.GetActor())
					{
						// Check if traced actor is also in sphere
						for (const FOverlapResult& Overlap : OverlapResults)
						{
							if (Overlap.GetActor() == TraceHit.GetActor())
							{
								// Both trace and sphere hit the same target!
								ValidTarget = TraceHit.GetActor();
								ValidHit = TraceHit;
								break;
							}
						}
					}
					
					if (ValidTarget)
					{
						// We have a valid target - apply damage
						float FinalDamage = Damage * GetDamageMultiplier();
						
						// Use the actor's TakeDamage method (will route to WP)
						FPointDamageEvent DamageEvent(FinalDamage, ValidHit, CameraForward, nullptr);
						ValidTarget->TakeDamage(FinalDamage, DamageEvent, nullptr, Owner);
						
						// Trigger hit stop
						if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
						{
							HitStopMgr->RequestLightHitStop();
						}
						
						#if WITH_EDITOR
						// Show successful hit
						DrawDebugSphere(GetWorld(), ValidHit.Location, 20.0f, 8, FColor::Green, false, 0.5f);
						DrawDebugLine(GetWorld(), CameraLocation, ValidHit.Location, FColor::Green, false, 0.5f, 0, 2.0f);
						DrawDebugSphere(GetWorld(), PlayerLocation, SphereRadius, 16, FColor::Blue, false, 0.5f);
						#endif
					}
					else
					{
						#if WITH_EDITOR
						// Show miss - draw debug info
						if (bTraceHit)
						{
							// Trace hit but not in sphere
							DrawDebugLine(GetWorld(), CameraLocation, TraceHit.Location, FColor::Yellow, false, 0.5f, 0, 2.0f);
							DrawDebugSphere(GetWorld(), TraceHit.Location, 20.0f, 8, FColor::Yellow, false, 0.5f);
						}
						else
						{
							// Complete miss
							DrawDebugLine(GetWorld(), CameraLocation, TraceEnd, FColor::Red, false, 0.5f, 0, 2.0f);
						}
						DrawDebugSphere(GetWorld(), PlayerLocation, SphereRadius, 16, FColor::Red, false, 0.5f);
						#endif
					}
				}
			else
			{
				// Fallback for no camera - use old method
				FVector Start = Owner->GetActorLocation() + FVector(0, 0, 50.0f);
				FVector End = Start + (Owner->GetActorForwardVector() * Range);
				
				FHitResult HitResult;
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(Owner);
				
				if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
				{
					if (AActor* HitActor = HitResult.GetActor())
					{
						// Apply damage to the hit actor
						float FinalDamage = Damage * GetDamageMultiplier();
						
						// Use the actor's TakeDamage method (will route to WP)
						FPointDamageEvent DamageEvent(FinalDamage, HitResult, Owner->GetActorForwardVector(), nullptr);
						HitActor->TakeDamage(FinalDamage, DamageEvent, nullptr, Owner);
						
						if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
						{
							HitStopMgr->RequestLightHitStop();
						}
					}
				}
			}
		}
		else
		{
			// Non-player owners use their actor location
			FVector Start = Owner->GetActorLocation();
			FVector End = Start + (Owner->GetActorForwardVector() * Range);
			
			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Owner);
			
			if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
			{
				if (AActor* HitActor = HitResult.GetActor())
				{
					// Apply damage to the hit actor
					float FinalDamage = Damage * GetDamageMultiplier();
					
					// Use the actor's TakeDamage method (will route to WP)
					FPointDamageEvent DamageEvent(FinalDamage, HitResult, Owner->GetActorForwardVector(), nullptr);
					HitActor->TakeDamage(FinalDamage, DamageEvent, nullptr, Owner);
					
					// Trigger hit stop
					if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
					{
						HitStopMgr->RequestLightHitStop();
					}
				}
			}
		}
	}
}
