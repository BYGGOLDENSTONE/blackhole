#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/UtilityAbility.h"
#include "HackerJumpAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UHackerJumpAbility : public UUtilityAbility
{
	GENERATED_BODY()
	
public:
	UHackerJumpAbility();
	
	// Jump parameters
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	float JumpVelocity = 1200.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	float AirControlBoost = 2.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	int32 MaxJumpCount = 2; // Allows double jump
	
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	float JumpCooldown = 0.5f; // Cooldown between jumps
	
	// Override CanExecute to check jump count
	virtual bool CanExecute() const override;
	
	// Override Execute to register with combo system
	virtual void Execute() override;
	
	// Public getters for UI
	int32 GetCurrentJumpCount() const { return CurrentJumpCount; }
	int32 GetMaxJumpCount() const { return MaxJumpCount; }
	float GetJumpCooldown() const { return JumpCooldown; }
	float GetTimeSinceLastJump() const { return TimeSinceLastJump; }
	
protected:
	// Override from UtilityAbility
	virtual void ApplyMovement(ACharacter* Character) override;
	virtual void BeginPlay() override;
	
private:
	// Track current jump count
	int32 CurrentJumpCount = 0;
	
	// Original air control value
	float OriginalAirControl = 0.0f;
	
	// Time since last jump (for cooldown between jumps)
	float TimeSinceLastJump = 0.0f;
	
	// Reset jump count when landing
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);
	
	// Check if character can jump
	bool CanJump() const;
	
	// Override tick to track jump cooldown
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};