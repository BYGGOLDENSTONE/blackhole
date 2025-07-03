#include "SystemFreezeAbilityComponent.h"
#include "../Attributes/WillPowerComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "../../Enemy/BaseEnemy.h"
#include "../../Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"

USystemFreezeAbilityComponent::USystemFreezeAbilityComponent()
{
	Cost = 10.0f;
	Cooldown = 3.0f;
	Range = 3000.0f;
	StunDuration = 2.0f;
}

void USystemFreezeAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (AActor* Owner = GetOwner())
	{
		WillPowerComponent = Owner->FindComponentByClass<UWillPowerComponent>();
	}
}

bool USystemFreezeAbilityComponent::CanExecute() const
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

void USystemFreezeAbilityComponent::Execute()
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
		FVector Start;
		FVector Forward;
		
		// Use camera for aiming if this is the player
		if (ABlackholePlayerCharacter* PlayerOwner = Cast<ABlackholePlayerCharacter>(Owner))
		{
			if (UCameraComponent* Camera = PlayerOwner->GetCameraComponent())
			{
				Start = Camera->GetComponentLocation();
				Forward = Camera->GetForwardVector();
			}
			else
			{
				// Fallback to character location
				Start = Owner->GetActorLocation();
				Forward = Owner->GetActorForwardVector();
			}
		}
		else
		{
			// Non-player owners use their actor location
			Start = Owner->GetActorLocation();
			Forward = Owner->GetActorForwardVector();
		}
		
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
					if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
					{
						if (UCharacterMovementComponent* MovementComp = HitCharacter->GetCharacterMovement())
						{
							MovementComp->DisableMovement();
							
							FTimerHandle UnstunTimerHandle;
							GetWorld()->GetTimerManager().SetTimer(UnstunTimerHandle, [MovementComp]()
							{
								if (IsValid(MovementComp))
								{
									MovementComp->SetMovementMode(MOVE_Walking);
								}
							}, StunDuration, false);
						}
					}
				}
			}
		}
		
		#if WITH_EDITOR
		// Only draw debug line for non-player owners (enemies)
		if (!Owner->IsA<ABlackholePlayerCharacter>())
		{
			DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, 1.0f, 0, 2.0f);
		}
		#endif
	}
}