#include "SlashAbilityComponent.h"
#include "../Attributes/StaminaComponent.h"
#include "../Attributes/IntegrityComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

USlashAbilityComponent::USlashAbilityComponent()
{
	Damage = 20.0f;
	Cost = 10.0f;
	Cooldown = 2.0f;
	Range = 200.0f;
}

void USlashAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (AActor* Owner = GetOwner())
	{
		StaminaComponent = Owner->FindComponentByClass<UStaminaComponent>();
	}
}

bool USlashAbilityComponent::CanExecute() const
{
	if (!Super::CanExecute())
	{
		return false;
	}
	
	if (StaminaComponent && !StaminaComponent->HasEnoughStamina(Cost))
	{
		return false;
	}
	
	return true;
}

void USlashAbilityComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute();
	
	if (StaminaComponent)
	{
		StaminaComponent->UseStamina(Cost);
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
				if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
				{
					TargetIntegrity->TakeDamage(Damage);
				}
			}
		}
		
		#if WITH_EDITOR
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 2.0f);
		#endif
	}
}