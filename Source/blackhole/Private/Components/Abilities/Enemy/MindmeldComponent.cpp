#include "Components/Abilities/Enemy/MindmeldComponent.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Systems/ResourceManager.h"
#include "Player/BlackholePlayerCharacter.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "DrawDebugHelpers.h"

UMindmeldComponent::UMindmeldComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	Cooldown = 0.0f;
	Range = 5000.0f;
	DrainRate = 1.0f;
	bIsMindmeldActive = false;
}

void UMindmeldComponent::BeginPlay()
{
	Super::BeginPlay();
	bIsMindmeldActive = false;
}

void UMindmeldComponent::Execute()
{
	bIsMindmeldActive = !bIsMindmeldActive;
	
	if (!bIsMindmeldActive)
	{
		StopMindmeld();
	}
	
	// Removed execute logging to reduce spam
}

void UMindmeldComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsMindmeldActive && TargetActor)
	{
		bool bHasLOS = HasLineOfSight();
		
		if (bHasLOS)
		{
			// Only corrupt player characters
			if (TargetActor->IsA<ABlackholePlayerCharacter>())
			{
				// Use ResourceManager for WP corruption
				if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
				{
					if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
					{
						// ADD WP corruption to player (positive value increases corruption)
						ResourceMgr->AddWillPower(DrainRate * DeltaTime);
						
						#if WITH_EDITOR
						if (AActor* Owner = GetOwner())
						{
							DrawDebugLine(GetWorld(), Owner->GetActorLocation() + FVector(0,0,50), 
										  TargetActor->GetActorLocation() + FVector(0,0,50), 
										  FColor::Purple, false, 0.0f, 0, 2.0f);
						}
						#endif
						
						// Removed tick logging to reduce spam
					}
					else
					{
						// Only log once
						static bool bLoggedOnce = false;
						if (!bLoggedOnce)
						{
							UE_LOG(LogTemp, Warning, TEXT("Mindmeld: ResourceManager not found!"));
							bLoggedOnce = true;
						}
					}
				}
			}
			else
			{
				// Target is not a player - stop mindmeld
				StopMindmeld();
			}
		}
		else
		{
			// Lost line of sight - stop mindmeld
			StopMindmeld();
		}
	}
	else if (bIsMindmeldActive)
	{
		// No target - stop mindmeld
		StopMindmeld();
	}
}

void UMindmeldComponent::SetTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void UMindmeldComponent::StopMindmeld()
{
	bIsMindmeldActive = false;
	TargetActor = nullptr;
}

bool UMindmeldComponent::HasLineOfSight() const
{
	if (!TargetActor || !GetOwner())
	{
		return false;
	}
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(TargetActor); // Ignore the target to see if anything blocks between
	
	FVector Start = GetOwner()->GetActorLocation() + FVector(0, 0, 50); // Raise start point to eye level
	FVector End = TargetActor->GetActorLocation() + FVector(0, 0, 50); // Target eye level
	
	// Check if there's anything blocking the path
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
	
	// If nothing was hit, we have line of sight
	// If something was hit, it's blocking our view
	return !bHit;
}