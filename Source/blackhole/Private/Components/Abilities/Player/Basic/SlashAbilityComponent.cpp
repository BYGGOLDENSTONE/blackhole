#include "Components/Abilities/Player/Basic/SlashAbilityComponent.h"
#include "Components/Attributes/StaminaComponent.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Systems/ComboSystem.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "WorldCollision.h"
#include "Systems/HitStopManager.h"

USlashAbilityComponent::USlashAbilityComponent()
{
	Damage = 20.0f;
	Cost = 10.0f; // Legacy compatibility
	StaminaCost = 10.0f; // New dual resource system
	WPCost = 15.0f; // For Hacker path slash
	HeatCost = 0.0f; // Slash doesn't use heat
	Cooldown = 2.0f;
	Range = 200.0f;
	
	// Mark as basic ability - not affected by ultimate system
	bIsBasicAbility = true;
}

void USlashAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache owner character
	OwnerCharacter = Cast<ABlackholePlayerCharacter>(GetOwner());
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
	
	// Prevent recursive combo execution
	if (bIsExecutingCombo)
	{
		return;
	}
	
	// Check if we're completing a combo
	bool bIsComboFinisher = false;
	if (OwnerCharacter)
	{
		if (UComboSystem* ComboSystem = OwnerCharacter->GetComboSystem())
		{
			// Check combo state before registering new input
			if (ComboSystem->IsInCombo())
			{
				const FActiveCombo& ActiveCombo = ComboSystem->GetActiveCombo();
				
				// Check which combo we're completing
				if (ActiveCombo.Pattern.ComboName == "PhantomStrike" && ActiveCombo.CurrentInputIndex == 1)
				{
					bIsExecutingCombo = true;
					ExecutePhantomStrike();
					bIsExecutingCombo = false;
					bIsComboFinisher = true;
				}
				else if (ActiveCombo.Pattern.ComboName == "AerialRave" && ActiveCombo.CurrentInputIndex == 1)
				{
					bIsExecutingCombo = true;
					ExecuteAerialRave();
					bIsExecutingCombo = false;
					bIsComboFinisher = true;
				}
				else if (ActiveCombo.Pattern.ComboName == "TempestBlade" && ActiveCombo.CurrentInputIndex == 2)
				{
					bIsExecutingCombo = true;
					ExecuteTempestBlade();
					bIsExecutingCombo = false;
					bIsComboFinisher = true;
				}
				else if (ActiveCombo.Pattern.ComboName == "BladeDance")
				{
					bIsExecutingCombo = true;
					ExecuteBladeDance(ActiveCombo.CurrentInputIndex + 1);
					bIsExecutingCombo = false;
					bIsComboFinisher = true;
				}
			}
		}
	}
	
	// If not a combo finisher, execute normal slash
	if (!bIsComboFinisher)
	{
		// Base class handles resource costs (stamina + WP) and cooldown
		Super::Execute();
		
		// Register with combo system
		if (OwnerCharacter)
		{
			if (UComboSystem* ComboSystem = OwnerCharacter->GetComboSystem())
			{
				ComboSystem->RegisterInput(EComboInputType::Slash);
			}
		}
		
		// Normal slash logic
		if (AActor* Owner = GetOwner())
		{
		FVector Start;
		FVector Forward;
		
		// Use camera for aiming if this is the player
		if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Owner))
		{
			if (UCameraComponent* Camera = PlayerOwner->GetCameraComponent())
			{
				Start = Camera->GetComponentLocation();
				Forward = Camera->GetForwardVector();
			}
			else
			{
				// Fallback to character location
				Start = Owner->GetActorLocation();
				Forward = Owner->GetActorForwardVector();
			}
		}
		else
		{
			// Non-player owners use their actor location
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
		
		#if WITH_EDITOR
		// Only draw debug line for non-player owners (enemies)
		if (!Owner->IsA<ABlackholePlayerCharacter>())
		{
			DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 2.0f);
		}
		#endif
		}
	}
}

void USlashAbilityComponent::ExecutePhantomStrike()
{
	// Dash + Slash combo: Teleport behind target for critical backstab
	if (!OwnerCharacter) return;
	
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector Forward = OwnerCharacter->GetActorForwardVector();
	FVector End = Start + (Forward * Range * 2.0f); // Extended range for combo
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
	{
		if (AActor* Target = HitResult.GetActor())
		{
			// Teleport behind target
			FVector TargetLocation = Target->GetActorLocation();
			FVector TargetForward = Target->GetActorForwardVector();
			FVector TeleportLocation = TargetLocation - (TargetForward * 150.0f);
			
			OwnerCharacter->SetActorLocation(TeleportLocation);
			OwnerCharacter->SetActorRotation(Target->GetActorRotation());
			
			// Apply critical damage
			if (UIntegrityComponent* TargetIntegrity = Target->FindComponentByClass<UIntegrityComponent>())
			{
				float CriticalDamage = Damage * 1.5f * GetDamageMultiplier();
				TargetIntegrity->TakeDamage(CriticalDamage);
				
				// Trigger critical hit stop
				if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
				{
					HitStopMgr->RequestCriticalHitStop();
				}
				
				// Visual effect
				DrawDebugSphere(GetWorld(), Target->GetActorLocation(), 50.0f, 12, FColor::Cyan, false, 1.0f);
			}
		}
	}
}

void USlashAbilityComponent::ExecuteAerialRave()
{
	// Jump + Slash combo: Downward slash with shockwave
	if (!IsValid(OwnerCharacter) || !IsValid(GetWorld()))
	{
		UE_LOG(LogTemp, Error, TEXT("SlashAbility: Invalid owner or world in ExecuteAerialRave"));
		return;
	}
	
	// Perform enhanced slash damage without calling Execute() to avoid recursion
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector Forward = OwnerCharacter->GetActorForwardVector();
	FVector End = Start + (Forward * Range);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
			{
				// Apply enhanced damage for aerial combo
				float AerialDamage = Damage * 1.25f * GetDamageMultiplier();
				TargetIntegrity->TakeDamage(AerialDamage);
				
				// Trigger medium hit stop for aerial attack
				if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
				{
					HitStopMgr->RequestMediumHitStop();
				}
			}
		}
	}
	
	// Create shockwave on landing
	FVector ShockwaveOrigin = OwnerCharacter->GetActorLocation();
	float ShockwaveRadius = 300.0f;
	
	// Find all enemies in shockwave radius
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ShockwaveRadius);
	
	if (GetWorld()->OverlapMultiByChannel(OverlapResults, ShockwaveOrigin, FQuat::Identity, 
		ECC_Pawn, CollisionShape))
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (AActor* HitActor = Result.GetActor())
			{
				if (HitActor != OwnerCharacter)
				{
					if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
					{
						// Shockwave damage
						float ShockwaveDamage = 50.0f * GetDamageMultiplier();
						TargetIntegrity->TakeDamage(ShockwaveDamage);
						
						// Light hit stop for shockwave hits
						if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
						{
							HitStopMgr->RequestLightHitStop();
						}
					}
				}
			}
		}
	}
	
	// Visual effect
	DrawDebugSphere(GetWorld(), ShockwaveOrigin, ShockwaveRadius, 24, FColor::Blue, false, 1.0f);
}

void USlashAbilityComponent::ExecuteTempestBlade()
{
	// Jump + Dash + Slash combo: Multi-hit teleport combo
	if (!OwnerCharacter) return;
	
	// Find up to 3 nearby enemies
	TArray<AActor*> Targets;
	float SearchRadius = 1000.0f;
	
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(SearchRadius);
	
	if (GetWorld()->OverlapMultiByChannel(OverlapResults, OwnerCharacter->GetActorLocation(), 
		FQuat::Identity, ECC_Pawn, CollisionShape))
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (AActor* HitActor = Result.GetActor())
			{
				if (HitActor != OwnerCharacter && Targets.Num() < 3)
				{
					Targets.Add(HitActor);
				}
			}
		}
	}
	
	// Teleport to each target and slash
	for (AActor* Target : Targets)
	{
		// Teleport to target
		FVector TeleportLocation = Target->GetActorLocation() + (FVector::UpVector * 50.0f);
		OwnerCharacter->SetActorLocation(TeleportLocation);
		
		// Apply damage
		if (UIntegrityComponent* TargetIntegrity = Target->FindComponentByClass<UIntegrityComponent>())
		{
			float ComboDamage = Damage * 0.8f * GetDamageMultiplier(); // 80% per hit
			TargetIntegrity->TakeDamage(ComboDamage);
			
			// Trigger heavy hit stop for combo finisher
			if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
			{
				HitStopMgr->RequestHeavyHitStop();
			}
		}
		
		// Visual trail
		DrawDebugLine(GetWorld(), OwnerCharacter->GetActorLocation(), Target->GetActorLocation(), 
			FColor::Purple, false, 1.0f, 0, 3.0f);
	}
}

void USlashAbilityComponent::ExecuteBladeDance(int32 HitCount)
{
	// Progressive slash combo with increasing damage
	if (!OwnerCharacter) return;
	
	// Calculate damage multiplier based on hit count
	float ComboMultiplier = 1.0f;
	switch (HitCount)
	{
		case 2: ComboMultiplier = 1.0f; break;
		case 3: ComboMultiplier = 1.25f; break;
		case 4: ComboMultiplier = 1.5f; break;
		case 5: ComboMultiplier = 2.0f; break;
		default: ComboMultiplier = 1.0f; break;
	}
	
	// Perform enhanced slash without calling Execute() to avoid recursion
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector Forward = OwnerCharacter->GetActorForwardVector();
	FVector End = Start + (Forward * Range);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
			{
				// Apply combo-enhanced damage
				float ComboDamage = Damage * ComboMultiplier * GetDamageMultiplier();
				TargetIntegrity->TakeDamage(ComboDamage);
				
				// Light hit stop for blade dance hits
				if (HitCount < 5 && GetWorld())
				{
					if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
					{
						HitStopMgr->RequestLightHitStop();
					}
				}
			}
		}
	}
	
	// Special effect for 5th hit
	if (HitCount >= 5)
	{
		// 360 degree finisher
		float FinisherRadius = 300.0f;
		TArray<FOverlapResult> OverlapResults;
		FCollisionShape CollisionShape = FCollisionShape::MakeSphere(FinisherRadius);
		
		if (GetWorld()->OverlapMultiByChannel(OverlapResults, OwnerCharacter->GetActorLocation(), 
			FQuat::Identity, ECC_Pawn, CollisionShape))
		{
			for (const FOverlapResult& Result : OverlapResults)
			{
				if (AActor* HitActor = Result.GetActor())
				{
					if (HitActor != OwnerCharacter)
					{
						if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
						{
							TargetIntegrity->TakeDamage(Damage * GetDamageMultiplier());
							
							// Heavy hit stop for blade dance finisher
							if (UHitStopManager* HitStopMgr = GetWorld()->GetSubsystem<UHitStopManager>())
							{
								HitStopMgr->RequestHeavyHitStop();
							}
						}
					}
				}
			}
		}
		
		// Visual effect
		DrawDebugSphere(GetWorld(), OwnerCharacter->GetActorLocation(), FinisherRadius, 32, FColor::Red, false, 2.0f);
	}
}

