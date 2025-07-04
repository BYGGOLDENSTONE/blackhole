#include "Components/Abilities/Player/Basic/KillAbilityComponent.h"
#include "Systems/ResourceManager.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"

UKillAbilityComponent::UKillAbilityComponent()
{
	Cost = 40.0f;
	Cooldown = 5.0f;
	Range = 3000.0f;
}

void UKillAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UKillAbilityComponent::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		return ResMgr->HasEnoughWillPower(Cost) && !ResMgr->IsOverheated();
	}
	
	return false;
}

void UKillAbilityComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	// Consume WP first
	if (UResourceManager* ResMgr = GetResourceManager())
	{
		if (!ResMgr->ConsumeWillPower(Cost))
		{
			return; // Failed to consume WP
		}
	}
	
	Super::Execute(); // This will add heat
	
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