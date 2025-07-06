#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/Abilities/Player/Forge/HeatShieldAbility.h"
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
	if (AActor* Owner = GetOwner())
	{
		// Check for Heat Shield first
		if (UHeatShieldAbility* HeatShield = Owner->FindComponentByClass<UHeatShieldAbility>())
		{
			if (HeatShield->IsShieldActive())
			{
				// Let the shield absorb damage
				DamageAmount = HeatShield->AbsorbDamage(DamageAmount);
				
				// If shield absorbed all damage, return
				if (DamageAmount <= 0.0f)
				{
					return;
				}
			}
		}
		
		// Check if owner has a block component and is currently blocking
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