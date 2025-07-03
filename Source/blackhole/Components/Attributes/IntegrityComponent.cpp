#include "IntegrityComponent.h"
#include "../Abilities/BlockComponent.h"
#include "GameFramework/Actor.h"

UIntegrityComponent::UIntegrityComponent()
{
	MaxValue = 100.0f;
	CurrentValue = 100.0f;
	RegenRate = 0.0f;
}

void UIntegrityComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UIntegrityComponent::TakeDamage(float DamageAmount)
{
	// Check if owner has a block component and is currently blocking
	if (AActor* Owner = GetOwner())
	{
		if (UBlockComponent* BlockComp = Owner->FindComponentByClass<UBlockComponent>())
		{
			if (BlockComp->IsBlocking())
			{
				// Reduce damage by 50% when blocking
				DamageAmount *= 0.5f;
			}
		}
	}
	
	ModifyValue(-FMath::Abs(DamageAmount));
}