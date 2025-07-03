#include "MindmeldComponent.h"
#include "../Attributes/WillPowerComponent.h"
#include "../../Player/BlackholePlayerCharacter.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UMindmeldComponent::UMindmeldComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	Cost = 0.0f;
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
	
	UE_LOG(LogTemp, Warning, TEXT("Mindmeld Execute: Active = %s"), bIsMindmeldActive ? TEXT("TRUE") : TEXT("FALSE"));
}

void UMindmeldComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsMindmeldActive && TargetActor)
	{
		bool bHasLOS = HasLineOfSight();
		
		if (bHasLOS)
		{
			// Only drain from player characters
			if (TargetActor->IsA<ABlackholePlayerCharacter>())
			{
				if (UWillPowerComponent* TargetWillPower = TargetActor->FindComponentByClass<UWillPowerComponent>())
				{
					TargetWillPower->DrainWillPower(DrainRate * DeltaTime);
					
					#if WITH_EDITOR
					if (AActor* Owner = GetOwner())
					{
						DrawDebugLine(GetWorld(), Owner->GetActorLocation() + FVector(0,0,50), 
									  TargetActor->GetActorLocation() + FVector(0,0,50), 
									  FColor::Purple, false, 0.0f, 0, 2.0f);
					}
					#endif
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Mindmeld: Target has no WillPowerComponent!"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Mindmeld: Target is not a player!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Mindmeld: No line of sight!"));
			StopMindmeld();
		}
	}
	else if (bIsMindmeldActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mindmeld: No target actor!"));
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