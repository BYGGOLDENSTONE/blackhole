#include "KillAbilityComponent.h"
#include "../Attributes/WillPowerComponent.h"
#include "../Attributes/IntegrityComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "../../Enemy/BaseEnemy.h"

UKillAbilityComponent::UKillAbilityComponent()
{
	Cost = 40.0f;
	Cooldown = 5.0f;
	Range = 3000.0f;
}

void UKillAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (AActor* Owner = GetOwner())
	{
		WillPowerComponent = Owner->FindComponentByClass<UWillPowerComponent>();
	}
}

bool UKillAbilityComponent::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	if (WillPowerComponent && !WillPowerComponent->HasEnoughWillPower(Cost))
	{
		return false;
	}
	
	return true;
}

void UKillAbilityComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute();
	
	if (WillPowerComponent)
	{
		WillPowerComponent->UseWillPower(Cost);
	}
	
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
		DrawDebugLine(GetWorld(), Start, End, FColor::Black, false, 1.0f, 0, 3.0f);
		#endif
	}
}