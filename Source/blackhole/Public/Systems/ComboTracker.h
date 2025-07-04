#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ComboTracker.generated.h"

class UAbilityComponent;
class UResourceManager;

UENUM(BlueprintType)
enum class EComboState : uint8
{
	Idle,
	Chaining,
	MiniComboReady,
	ComboSurgeReady
};

USTRUCT(BlueprintType)
struct FComboReward
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	int32 WPReward = 0;
	
	UPROPERTY(BlueprintReadOnly)
	float HeatReduction = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float DamageMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FComboChain
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<TSubclassOf<UAbilityComponent>> AbilitySequence;
	
	UPROPERTY()
	float LastAbilityTime = 0.0f;
	
	UPROPERTY()
	int32 ChainLength = 0;
	
	void Reset()
	{
		AbilitySequence.Empty();
		LastAbilityTime = 0.0f;
		ChainLength = 0;
	}
	
	bool IsDistinctAbility(TSubclassOf<UAbilityComponent> NewAbility) const
	{
		return !AbilitySequence.Contains(NewAbility);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComboReward, EComboState, ComboType, const FComboReward&, Reward);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboStateChanged, EComboState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboChainUpdated, int32, ChainLength);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UComboTracker : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UComboTracker();
	
	// Component overrides
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Events
	UPROPERTY(BlueprintAssignable, Category = "Combo Events")
	FOnComboReward OnComboReward;
	
	UPROPERTY(BlueprintAssignable, Category = "Combo Events")
	FOnComboStateChanged OnComboStateChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Combo Events")
	FOnComboChainUpdated OnComboChainUpdated;
	
	// Register ability use
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void RegisterAbilityUse(UAbilityComponent* Ability);
	
	// Get current combo state
	UFUNCTION(BlueprintPure, Category = "Combo")
	EComboState GetCurrentState() const { return CurrentState; }
	
	// Get current chain length
	UFUNCTION(BlueprintPure, Category = "Combo")
	int32 GetChainLength() const { return CurrentChain.ChainLength; }
	
	// Get next damage multiplier
	UFUNCTION(BlueprintPure, Category = "Combo")
	float GetNextDamageMultiplier() const { return NextDamageMultiplier; }
	
	// Chain window duration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	float ChainWindowDuration = 2.0f;
	
private:
	// Current combo state
	EComboState CurrentState = EComboState::Idle;
	
	// Current chain tracking
	FComboChain CurrentChain;
	
	// Next damage multiplier (from combo surge)
	float NextDamageMultiplier = 1.0f;
	
	// Resource manager reference
	UPROPERTY()
	UResourceManager* ResourceManager;
	
	// Timer for chain window
	FTimerHandle ChainWindowTimer;
	
	// Process combo rewards
	void ProcessComboReward();
	
	// Reset combo chain
	void ResetCombo();
	
	// Apply combo reward
	void ApplyReward(const FComboReward& Reward);
	
	// Update combo state
	void UpdateComboState();
};