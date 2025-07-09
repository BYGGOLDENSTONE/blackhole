#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Components/Abilities/Player/Forge/HeatShieldAbility.h"
#include "GameFramework/Actor.h"
#include "Config/GameplayConfig.h"

UIntegrityComponent::UIntegrityComponent()
{
	MaxValue = GameplayConfig::Attributes::Integrity::MAX_VALUE;
	CurrentValue = GameplayConfig::Attributes::Integrity::MAX_VALUE;
	RegenRate = GameplayConfig::Attributes::DEFAULT_REGEN_RATE;
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
				// Reduce damage by configured amount when blocking
				DamageAmount *= GameplayConfig::Attributes::Integrity::BLOCK_DAMAGE_REDUCTION;
			}
		}
	}
	
	ModifyValue(-FMath::Abs(DamageAmount));
}