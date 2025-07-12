#include "Components/Abilities/Player/Basic/SlashAbilityComponent.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "GameFramework/Actor.h"
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
				// Step 1: Find what the crosshair is pointing at with a line trace
				FVector CameraLocation = Camera->GetComponentLocation();
				FVector CameraForward = Camera->GetForwardVector();
				FVector TraceEnd = CameraLocation + (CameraForward * Range);
				
				FHitResult AimHit;
				FCollisionQueryParams AimParams;
				AimParams.AddIgnoredActor(Owner);
				
				// Line trace to find target
				bool bHit = GetWorld()->LineTraceSingleByChannel(AimHit, CameraLocation, TraceEnd, ECC_Pawn, AimParams);
				
				if (bHit && AimHit.GetActor())
				{
					// We hit something - check if it's a valid target
					if (UIntegrityComponent* TargetIntegrity = AimHit.GetActor()->FindComponentByClass<UIntegrityComponent>())
					{
						// Apply damage with survivor buff multiplier
						float FinalDamage = Damage * GetDamageMultiplier();
						TargetIntegrity->TakeDamage(FinalDamage);
						
						// Trigger hit stop
						if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
						{
							HitStopMgr->RequestLightHitStop();
						}
						
						#if WITH_EDITOR
						// Show hit location
						DrawDebugSphere(GetWorld(), AimHit.Location, 20.0f, 8, FColor::Red, false, 0.5f);
						DrawDebugLine(GetWorld(), CameraLocation, AimHit.Location, FColor::Green, false, 0.5f, 0, 2.0f);
						#endif
					}
				}
				else
				{
					#if WITH_EDITOR
					// Show miss
					DrawDebugLine(GetWorld(), CameraLocation, TraceEnd, FColor::Red, false, 0.5f, 0, 2.0f);
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
						if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
						{
							float FinalDamage = Damage * GetDamageMultiplier();
							TargetIntegrity->TakeDamage(FinalDamage);
							
							if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
							{
								HitStopMgr->RequestLightHitStop();
							}
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
					if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
					{
						// Apply damage with survivor buff multiplier
						float FinalDamage = Damage * GetDamageMultiplier();
						TargetIntegrity->TakeDamage(FinalDamage);
						
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
}
