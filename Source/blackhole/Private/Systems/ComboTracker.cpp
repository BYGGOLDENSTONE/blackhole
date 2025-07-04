#include "Systems/ComboTracker.h"
#include "Systems/ResourceManager.h"
#include "Components/Abilities/Player/AbilityComponent.h"
#include "Components/Abilities/Player/UtilityAbility.h"
#include "Engine/World.h"
#include "TimerManager.h"

UComboTracker::UComboTracker()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Don't need to tick every frame
}

void UComboTracker::BeginPlay()
{
	Super::BeginPlay();
	
	// Get resource manager
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		ResourceManager = GameInstance->GetSubsystem<UResourceManager>();
	}
}

void UComboTracker::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Check if combo window has expired
	if (CurrentState != EComboState::Idle && CurrentChain.LastAbilityTime > 0.0f)
	{
		float TimeSinceLastAbility = GetWorld()->GetTimeSeconds() - CurrentChain.LastAbilityTime;
		if (TimeSinceLastAbility > ChainWindowDuration)
		{
			// Window expired, process any pending rewards
			if (CurrentChain.ChainLength >= 2)
			{
				ProcessComboReward();
			}
			else
			{
				ResetCombo();
			}
		}
	}
}

void UComboTracker::RegisterAbilityUse(UAbilityComponent* Ability)
{
	if (!Ability)
	{
		return;
	}
	
	// Don't count utility abilities in combos
	if (Cast<UUtilityAbility>(Ability))
	{
		return;
	}
	
	TSubclassOf<UAbilityComponent> AbilityClass = Ability->GetClass();
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	// Check if this is within combo window
	if (CurrentState == EComboState::Idle || 
		(CurrentTime - CurrentChain.LastAbilityTime) > ChainWindowDuration)
	{
		// Start new chain
		CurrentChain.Reset();
		CurrentState = EComboState::Chaining;
	}
	
	// Check if this is a distinct ability
	if (CurrentChain.IsDistinctAbility(AbilityClass))
	{
		// Add to chain
		CurrentChain.AbilitySequence.Add(AbilityClass);
		CurrentChain.ChainLength++;
		CurrentChain.LastAbilityTime = CurrentTime;
		
		// Update state based on chain length
		UpdateComboState();
		
		// Broadcast chain update
		OnComboChainUpdated.Broadcast(CurrentChain.ChainLength);
		
		UE_LOG(LogTemp, Log, TEXT("ComboTracker: Chain updated to %d abilities"), CurrentChain.ChainLength);
	}
	else
	{
		// Same ability used, combo broken
		UE_LOG(LogTemp, Log, TEXT("ComboTracker: Combo broken - same ability used"));
		
		// Process any pending rewards before resetting
		if (CurrentChain.ChainLength >= 2)
		{
			ProcessComboReward();
		}
		else
		{
			ResetCombo();
		}
	}
}

void UComboTracker::ProcessComboReward()
{
	FComboReward Reward;
	EComboState RewardType = CurrentState;
	
	// Determine reward based on chain length
	if (CurrentChain.ChainLength >= 3)
	{
		// Combo Surge (3+ distinct abilities)
		Reward.WPReward = 10;
		Reward.DamageMultiplier = 1.5f;
		NextDamageMultiplier = 1.5f; // Store for next ability
		RewardType = EComboState::ComboSurgeReady;
		
		UE_LOG(LogTemp, Log, TEXT("ComboTracker: COMBO SURGE! +10 WP, +50%% next damage"));
	}
	else if (CurrentChain.ChainLength == 2)
	{
		// Mini Combo (2 distinct abilities)
		// Player chooses between WP or Heat reduction
		Reward.WPReward = 2;
		Reward.HeatReduction = 3.0f;
		RewardType = EComboState::MiniComboReady;
		
		UE_LOG(LogTemp, Log, TEXT("ComboTracker: Mini Combo! +2 WP or -3 Heat"));
	}
	
	// Apply reward
	ApplyReward(Reward);
	
	// Broadcast reward event
	OnComboReward.Broadcast(RewardType, Reward);
	
	// Reset combo
	ResetCombo();
}

void UComboTracker::ResetCombo()
{
	CurrentChain.Reset();
	CurrentState = EComboState::Idle;
	NextDamageMultiplier = 1.0f;
	
	// Clear timer
	GetWorld()->GetTimerManager().ClearTimer(ChainWindowTimer);
	
	// Broadcast state change
	OnComboStateChanged.Broadcast(CurrentState);
}

void UComboTracker::ApplyReward(const FComboReward& Reward)
{
	if (!ResourceManager)
	{
		return;
	}
	
	// Apply WP reward
	if (Reward.WPReward > 0)
	{
		ResourceManager->AddWillPower(Reward.WPReward);
	}
	
	// Apply heat reduction
	if (Reward.HeatReduction > 0.0f)
	{
		float CurrentHeat = ResourceManager->GetCurrentHeat();
		float NewHeat = FMath::Max(0.0f, CurrentHeat - Reward.HeatReduction);
		
		// Add negative heat to reduce it
		ResourceManager->AddHeat(NewHeat - CurrentHeat);
	}
}

void UComboTracker::UpdateComboState()
{
	EComboState NewState = CurrentState;
	
	if (CurrentChain.ChainLength >= 3)
	{
		NewState = EComboState::ComboSurgeReady;
	}
	else if (CurrentChain.ChainLength == 2)
	{
		NewState = EComboState::MiniComboReady;
	}
	else
	{
		NewState = EComboState::Chaining;
	}
	
	if (NewState != CurrentState)
	{
		CurrentState = NewState;
		OnComboStateChanged.Broadcast(CurrentState);
	}
}