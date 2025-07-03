#include "DodgeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

UDodgeComponent::UDodgeComponent()
{
	Cost = 0.0f;
	Cooldown = 2.0f;
	DodgeDistance = 300.0f;
	DodgeDuration = 0.3f;
}

void UDodgeComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDodgeComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute();
	
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		FVector DodgeDirection = Character->GetActorRightVector();
		
		if (Character->GetVelocity().Size() > 0.1f)
		{
			DodgeDirection = Character->GetVelocity().GetSafeNormal();
		}
		
		FVector LaunchVelocity = DodgeDirection * (DodgeDistance / DodgeDuration);
		Character->LaunchCharacter(LaunchVelocity, true, false);
	}
}