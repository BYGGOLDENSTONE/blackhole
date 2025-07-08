#include "Components/Abilities/Player/Basic/KillAbilityComponent.h"
#include "Systems/ResourceManager.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "GameFramework/Actor.h"
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
	Cost = 0.0f;
	StaminaCost = 0.0f;
	WPCost = 0.0f;
	HeatCost = 0.0f;
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
				// Check if the target is an enemy (has tag "Enemy" or is ABaseEnemy)
				if (HitActor->ActorHasTag("Enemy") || HitActor->IsA<ABaseEnemy>())
				{
					if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
					{
						// Test damage with buffs
						float BaseDamage = 100.0f;
						float FinalDamage = BaseDamage * GetDamageMultiplier();
						
						// Show damage value on screen
						if (GEngine)
						{
							FString DamageText = FString::Printf(TEXT("Kill Damage: %.0f (Base: %.0f, Multiplier: %.1fx)"), 
								FinalDamage, BaseDamage, GetDamageMultiplier());
							GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, DamageText);
							
							UE_LOG(LogTemp, Warning, TEXT("Kill Ability: %s"), *DamageText);
						}
						
						// Instant kill for now
						TargetIntegrity->SetCurrentValue(0.0f);
					}
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
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("ULTIMATE KILL: DEATH WAVE!"));
	}
	
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
					if (UIntegrityComponent* TargetIntegrity = Target->FindComponentByClass<UIntegrityComponent>())
					{
						TargetIntegrity->SetCurrentValue(0.0f);
						KillCount++;
					}
				}
			}
		}
		
		if (GEngine)
		{
			FString KillText = FString::Printf(TEXT("Ultimate Kill: Eliminated %d enemies!"), KillCount);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, KillText);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *KillText);
		}
		
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