#include "BlockComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

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
	
	GetWorld()->GetTimerManager().SetTimer(BlockTimerHandle, this, &UBlockComponent::StopBlocking, BlockDuration, false);
}

void UBlockComponent::StopBlocking()
{
	bIsBlocking = false;
}