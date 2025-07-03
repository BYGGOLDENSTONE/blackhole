#include "SmashAbilityComponent.h"
#include "../Attributes/IntegrityComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

USmashAbilityComponent::USmashAbilityComponent()
{
	Damage = 10.0f;
	Cost = 0.0f;
	Cooldown = 1.5f;
	Range = 200.0f;
}

void USmashAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USmashAbilityComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute();
	
	if (AActor* Owner = GetOwner())
	{
		FVector Start = Owner->GetActorLocation();
		FVector Forward = Owner->GetActorForwardVector();
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
					TargetIntegrity->TakeDamage(Damage);
				}
			}
		}
		
		#if WITH_EDITOR
		DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 2.0f);
		#endif
	}
}