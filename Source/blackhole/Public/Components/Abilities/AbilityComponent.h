#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/ResourceConsumer.h"
#include "AbilityComponent.generated.h"

class UResourceManager;
class UThresholdManager;

UENUM(BlueprintType)
enum class EAbilityState : uint8
{
	Ready,
	Executing,
	Cooldown,
	Disabled
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float Cooldown;


	// WP Energy System - Abilities consume WP as energy/mana
	// Higher WPCost = More energy consumed when using ability
	// Basic abilities (slash, dash, jump) have 0 WP cost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (DisplayName = "WP Energy Cost"))
	float WPCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float Range;

	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	float CurrentCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	bool bIsOnCooldown;
	
	// Improved state tracking
	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	EAbilityState CurrentState = EAbilityState::Ready;
	
	// Tick optimization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization")
	bool bTickOnlyWhenActive = true;
	
	// Track if ability is in ultimate mode
	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	bool bIsInUltimateMode;
	
	// Track if this is a basic ability (not affected by ultimate system)
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	bool bIsBasicAbility;
	
	// Track if ability has been permanently disabled (used in ultimate mode)
	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	bool bIsDisabled;

	// Common ultimate properties (can be overridden by specific abilities)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Base", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float UltimateRangeMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Base", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float UltimateDamageMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Base", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float UltimateDurationMultiplier = 1.5f;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual bool CanExecute() const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual void Execute();
	
	// Execute the ultimate version of this ability
	virtual void ExecuteUltimate();

	UFUNCTION(BlueprintCallable, Category = "Ability")
	float GetCooldownRemaining() const { return CurrentCooldown; }

	UFUNCTION(BlueprintCallable, Category = "Ability")
	float GetCooldownPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool IsOnCooldown() const { return bIsOnCooldown; }

	UFUNCTION(BlueprintCallable, Category = "Ability")
	float GetRange() const { return Range; }


	UFUNCTION(BlueprintCallable, Category = "Ability")
	float GetWPCost() const { return WPCost; }

	
	// Ultimate mode support
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SetUltimateMode(bool bEnabled);
	
	UFUNCTION(BlueprintPure, Category = "Ability")
	bool IsInUltimateMode() const { return bIsInUltimateMode; }
	
	UFUNCTION(BlueprintPure, Category = "Ability")
	bool IsBasicAbility() const { return bIsBasicAbility; }
	
	// Disable/Enable ability permanently
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SetDisabled(bool bNewDisabled) 
	{ 
		bIsDisabled = bNewDisabled; 
		UE_LOG(LogTemp, Error, TEXT("Ability %s: SetDisabled called with %s"), 
			*GetName(), bNewDisabled ? TEXT("TRUE") : TEXT("FALSE"));
	}
	
	UFUNCTION(BlueprintPure, Category = "Ability")
	bool IsDisabled() const { return bIsDisabled; }
	
	// State management (made public for ThresholdManager)
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SetAbilityState(EAbilityState NewState);
	
	// Setters for protected properties (for data table system)
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SetRange(float NewRange) { Range = NewRange; }
	
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SetCooldown(float NewCooldown) { Cooldown = NewCooldown; }

protected:
	void StartCooldown();
	void ResetCooldown();
	
	// Cached reference to resource manager
	UPROPERTY()
	UResourceManager* ResourceManager;
	
	// Helper to get resource manager
	UResourceManager* GetResourceManager() const;
	
	// Apply survivor buff modifiers
	virtual float GetDamageMultiplier() const;
	virtual float GetCooldownWithReduction() const;
	
	// Cached threshold manager
	UPROPERTY()
	UThresholdManager* ThresholdManager;
	
	// Optimized tick management
	void SetTickEnabled(bool bEnabled);
	
	// Resource validation helpers
	bool ValidateResources() const;
	bool ConsumeAbilityResources();
	
	// Get resource consumer interface from owner
	IResourceConsumer* GetOwnerResourceInterface() const;
};