#include "Components/Abilities/Enemy/BlockComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"

UBlockComponent::UBlockComponent()
{
	Cost = 0.0f;
	Cooldown = 2.0f;
	BlockDuration = 1.0f;
	bIsBlocking = false;
}

void UBlockComponent::BeginPlay()
{
	Super::BeginPlay();
	bIsBlocking = false;
}

void UBlockComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute();
	
	bIsBlocking = true;
	
	// Show shield mesh if available
	if (AActor* Owner = GetOwner())
	{
		if (UStaticMeshComponent* ShieldMesh = Owner->FindComponentByClass<UStaticMeshComponent>())
		{
			// Check if this is actually a shield mesh by checking its name
			if (ShieldMesh->GetName().Contains("Shield"))
			{
				ShieldMesh->SetVisibility(true);
			}
		}
	}
	
	GetWorld()->GetTimerManager().SetTimer(BlockTimerHandle, this, &UBlockComponent::StopBlocking, BlockDuration, false);
}

void UBlockComponent::StopBlocking()
{
	bIsBlocking = false;
	
	// Hide shield mesh if available
	if (AActor* Owner = GetOwner())
	{
		if (UStaticMeshComponent* ShieldMesh = Owner->FindComponentByClass<UStaticMeshComponent>())
		{
			// Check if this is actually a shield mesh by checking its name
			if (ShieldMesh->GetName().Contains("Shield"))
			{
				ShieldMesh->SetVisibility(false);
			}
		}
	}
}