#include "Components/StatusEffectComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UStatusEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAllStatusEffects();
	Super::EndPlay(EndPlayReason);
}

void UStatusEffectComponent::ApplyStatusEffect(EStatusEffectType EffectType, float Duration, float Magnitude, bool bAllowStacking)
{
	if (EffectType == EStatusEffectType::None) return;
	
	// Check immunities
	if (Immunities & (1 << (int32)EffectType))
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect: %s is immune to effect type %d"), 
			*GetOwner()->GetName(), (int32)EffectType);
		return;
	}
	
	// Check if effect already exists
	if (FStatusEffect* ExistingEffect = ActiveEffects.Find(EffectType))
	{
		if (bAllowStacking && ExistingEffect->StackCount < ExistingEffect->MaxStacks)
		{
			ExistingEffect->StackCount++;
			ExistingEffect->Magnitude = FMath::Max(ExistingEffect->Magnitude, Magnitude);
		}
		
		// Refresh duration if new duration is longer
		if (Duration > GetEffectRemainingDuration(EffectType))
		{
			// Clear old timer
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(ExistingEffect->DurationTimer);
				
				// Set new timer
				if (Duration > 0.0f && !ExistingEffect->bIsInfinite)
				{
					FTimerDelegate TimerDel;
					TimerDel.BindUObject(this, &UStatusEffectComponent::OnEffectExpired, EffectType);
					World->GetTimerManager().SetTimer(ExistingEffect->DurationTimer, TimerDel, Duration, false);
				}
			}
			ExistingEffect->Duration = Duration;
		}
		return;
	}
	
	// Create new effect
	FStatusEffect NewEffect;
	NewEffect.Type = EffectType;
	NewEffect.Duration = Duration;
	NewEffect.Magnitude = Magnitude;
	NewEffect.bIsInfinite = (Duration <= 0.0f);
	
	// Apply effect logic
	ApplyEffectLogic(EffectType, Magnitude);
	
	// Set timer for non-infinite effects
	if (!NewEffect.bIsInfinite && Duration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerDelegate TimerDel;
			TimerDel.BindUObject(this, &UStatusEffectComponent::OnEffectExpired, EffectType);
			World->GetTimerManager().SetTimer(NewEffect.DurationTimer, TimerDel, Duration, false);
		}
	}
	
	// Store effect
	ActiveEffects.Add(EffectType, NewEffect);
	
	// Broadcast event
	OnStatusEffectApplied.Broadcast(EffectType, Duration);
	
	UE_LOG(LogTemp, Warning, TEXT("StatusEffect: Applied %d to %s for %.1fs"), 
		(int32)EffectType, *GetOwner()->GetName(), Duration);
}

void UStatusEffectComponent::RemoveStatusEffect(EStatusEffectType EffectType)
{
	FStatusEffect* Effect = ActiveEffects.Find(EffectType);
	if (!Effect) return;
	
	// Clear timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Effect->DurationTimer);
	}
	
	// Remove effect logic
	RemoveEffectLogic(EffectType);
	
	// Remove from map
	ActiveEffects.Remove(EffectType);
	
	// Broadcast event
	OnStatusEffectRemoved.Broadcast(EffectType);
	
	UE_LOG(LogTemp, Warning, TEXT("StatusEffect: Removed %d from %s"), 
		(int32)EffectType, *GetOwner()->GetName());
}

void UStatusEffectComponent::ClearAllStatusEffects()
{
	TArray<EStatusEffectType> EffectsToRemove;
	for (const auto& EffectPair : ActiveEffects)
	{
		EffectsToRemove.Add(EffectPair.Key);
	}
	
	for (EStatusEffectType EffectType : EffectsToRemove)
	{
		RemoveStatusEffect(EffectType);
	}
}

bool UStatusEffectComponent::HasStatusEffect(EStatusEffectType EffectType) const
{
	return ActiveEffects.Contains(EffectType);
}

float UStatusEffectComponent::GetEffectRemainingDuration(EStatusEffectType EffectType) const
{
	const FStatusEffect* Effect = ActiveEffects.Find(EffectType);
	if (!Effect) return 0.0f;
	
	if (Effect->bIsInfinite) return -1.0f;
	
	if (UWorld* World = GetWorld())
	{
		return World->GetTimerManager().GetTimerRemaining(Effect->DurationTimer);
	}
	
	return 0.0f;
}

float UStatusEffectComponent::GetEffectMagnitude(EStatusEffectType EffectType) const
{
	const FStatusEffect* Effect = ActiveEffects.Find(EffectType);
	return Effect ? Effect->Magnitude : 0.0f;
}

bool UStatusEffectComponent::CanMove() const
{
	return !HasStatusEffect(EStatusEffectType::Stagger) && 
	       !HasStatusEffect(EStatusEffectType::Stun) &&
	       !HasStatusEffect(EStatusEffectType::Freeze) &&
	       !HasStatusEffect(EStatusEffectType::Knockdown) &&
	       !HasStatusEffect(EStatusEffectType::Dead);
}

bool UStatusEffectComponent::CanAct() const
{
	return !HasStatusEffect(EStatusEffectType::Stagger) && 
	       !HasStatusEffect(EStatusEffectType::Stun) &&
	       !HasStatusEffect(EStatusEffectType::Dead);
}

TArray<EStatusEffectType> UStatusEffectComponent::GetActiveEffects() const
{
	TArray<EStatusEffectType> Effects;
	for (const auto& EffectPair : ActiveEffects)
	{
		Effects.Add(EffectPair.Key);
	}
	return Effects;
}

void UStatusEffectComponent::OnEffectExpired(EStatusEffectType EffectType)
{
	RemoveStatusEffect(EffectType);
}

void UStatusEffectComponent::ApplyEffectLogic(EStatusEffectType EffectType, float Magnitude)
{
	ACharacter* CharOwner = Cast<ACharacter>(GetOwner());
	if (!CharOwner) return;
	
	switch (EffectType)
	{
		case EStatusEffectType::Stagger:
		case EStatusEffectType::Stun:
		case EStatusEffectType::Freeze:
		case EStatusEffectType::Knockdown:
			UpdateMovementState();
			UpdateInputState();
			// Slow animation
			if (CharOwner->GetMesh())
			{
				CharOwner->GetMesh()->GlobalAnimRateScale = 0.3f;
			}
			break;
			
		case EStatusEffectType::Slow:
			if (UCharacterMovementComponent* Movement = CharOwner->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed *= Magnitude;
			}
			break;
			
		case EStatusEffectType::SpeedBoost:
			if (UCharacterMovementComponent* Movement = CharOwner->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed *= Magnitude;
			}
			break;
			
		case EStatusEffectType::Dead:
			UpdateMovementState();
			UpdateInputState();
			break;
	}
}

void UStatusEffectComponent::RemoveEffectLogic(EStatusEffectType EffectType)
{
	ACharacter* CharOwner = Cast<ACharacter>(GetOwner());
	if (!CharOwner) return;
	
	switch (EffectType)
	{
		case EStatusEffectType::Stagger:
		case EStatusEffectType::Stun:
		case EStatusEffectType::Freeze:
		case EStatusEffectType::Knockdown:
			UpdateMovementState();
			UpdateInputState();
			// Restore animation
			if (CharOwner->GetMesh())
			{
				CharOwner->GetMesh()->GlobalAnimRateScale = 1.0f;
			}
			break;
			
		case EStatusEffectType::Slow:
		case EStatusEffectType::SpeedBoost:
			// Restore base speed - this needs to be handled by the character class
			// as we don't know the original speed
			break;
	}
}

void UStatusEffectComponent::UpdateMovementState()
{
	ACharacter* CharOwner = Cast<ACharacter>(GetOwner());
	if (!CharOwner || !CharOwner->GetCharacterMovement()) return;
	
	if (CanMove())
	{
		CharOwner->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		CharOwner->GetCharacterMovement()->DisableMovement();
	}
}

void UStatusEffectComponent::UpdateInputState()
{
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!PawnOwner) return;
	
	APlayerController* PC = Cast<APlayerController>(PawnOwner->GetController());
	if (!PC) return;
	
	if (CanAct())
	{
		PawnOwner->EnableInput(PC);
	}
	else
	{
		PawnOwner->DisableInput(PC);
	}
}