#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusEffectComponent.generated.h"

UENUM(BlueprintType)
enum class EStatusEffectType : uint8
{
	None = 0,
	Stagger = 1,
	Stun = 2,
	Slow = 3,
	Burn = 4,
	Freeze = 5,
	Knockdown = 6,
	Invulnerable = 7,
	SpeedBoost = 8,
	DamageBoost = 9,
	Shield = 10,
	Dead = 255
};

// Enum for effect stacking behavior
UENUM(BlueprintType)
enum class EEffectStackingRule : uint8
{
	Replace,        // New effect replaces old
	Stack,          // Effects stack up to max
	Refresh,        // Reset duration to new value
	RefreshExtend,  // Add new duration to existing
	Ignore          // Ignore new effect if one exists
};

USTRUCT(BlueprintType)
struct FStatusEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EStatusEffectType Type = EStatusEffectType::None;
	
	UPROPERTY(BlueprintReadWrite)
	float Duration = 0.0f;
	
	UPROPERTY(BlueprintReadWrite)
	float Magnitude = 1.0f; // For effects like slow (0.5 = 50% speed)
	
	UPROPERTY(BlueprintReadWrite)
	bool bIsInfinite = false;
	
	// For stacking effects
	UPROPERTY(BlueprintReadWrite)
	int32 StackCount = 1;
	
	UPROPERTY(BlueprintReadWrite)
	int32 MaxStacks = 1;
	
	// Source tracking
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> Source;
	
	// Priority for conflicting effects (higher = more important)
	UPROPERTY(BlueprintReadWrite)
	int32 Priority = 0;
	
	// Timer handle for duration
	FTimerHandle DurationTimer;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatusEffectApplied, EStatusEffectType, EffectType, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatusEffectRemoved, EStatusEffectType, EffectType);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusEffectComponent();

	// Apply a status effect
	UFUNCTION(BlueprintCallable, Category = "Status Effects")
	void ApplyStatusEffect(EStatusEffectType EffectType, float Duration, float Magnitude = 1.0f, bool bAllowStacking = false, AActor* Source = nullptr, int32 Priority = 0);
	
	// Remove a specific status effect
	UFUNCTION(BlueprintCallable, Category = "Status Effects")
	void RemoveStatusEffect(EStatusEffectType EffectType);
	
	// Remove all status effects
	UFUNCTION(BlueprintCallable, Category = "Status Effects")
	void ClearAllStatusEffects();
	
	// Check if has specific effect
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	bool HasStatusEffect(EStatusEffectType EffectType) const;
	
	// Get remaining duration
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	float GetEffectRemainingDuration(EStatusEffectType EffectType) const;
	
	// Get effect magnitude
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	float GetEffectMagnitude(EStatusEffectType EffectType) const;
	
	// Check specific states
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	bool IsStaggered() const { return HasStatusEffect(EStatusEffectType::Stagger); }
	
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	bool IsStunned() const { return HasStatusEffect(EStatusEffectType::Stun); }
	
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	bool IsSlowed() const { return HasStatusEffect(EStatusEffectType::Slow); }
	
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	bool IsInvulnerable() const { return HasStatusEffect(EStatusEffectType::Invulnerable); }
	
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	bool IsDead() const { return HasStatusEffect(EStatusEffectType::Dead); }
	
	// Can the owner move?
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	bool CanMove() const;
	
	// Can the owner act (use abilities)?
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	bool CanAct() const;
	
	// Get all active effects
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	TArray<EStatusEffectType> GetActiveEffects() const;
	
	// Remove all effects from a specific source
	UFUNCTION(BlueprintCallable, Category = "Status Effects")
	void RemoveEffectsFromSource(AActor* Source);
	
	// Get effect source
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	AActor* GetEffectSource(EStatusEffectType EffectType) const;
	
	// Get effect priority
	UFUNCTION(BlueprintPure, Category = "Status Effects")
	int32 GetEffectPriority(EStatusEffectType EffectType) const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Status Effects")
	FOnStatusEffectApplied OnStatusEffectApplied;
	
	UPROPERTY(BlueprintAssignable, Category = "Status Effects")
	FOnStatusEffectRemoved OnStatusEffectRemoved;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Active status effects
	UPROPERTY()
	TMap<EStatusEffectType, FStatusEffect> ActiveEffects;
	
	// Effect immunities
	UPROPERTY(EditAnywhere, Category = "Status Effects", meta = (Bitmask, BitmaskEnum = "EStatusEffectType"))
	int32 Immunities = 0;
	
	// Handle effect expiration
	void OnEffectExpired(EStatusEffectType EffectType);
	
	// Apply effect-specific logic
	void ApplyEffectLogic(EStatusEffectType EffectType, float Magnitude);
	void RemoveEffectLogic(EStatusEffectType EffectType);
	
	// Update movement and input based on effects
	void UpdateMovementState();
	void UpdateInputState();
	
	// Get stacking rule for effect type
	EEffectStackingRule GetStackingRule(EStatusEffectType EffectType) const;
};