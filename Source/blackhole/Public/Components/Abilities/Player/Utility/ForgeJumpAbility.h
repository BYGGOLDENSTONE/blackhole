#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/UtilityAbility.h"
#include "ForgeJumpAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UForgeJumpAbility : public UUtilityAbility
{
	GENERATED_BODY()
	
public:
	UForgeJumpAbility();
	
	// Jump parameters
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	float JumpVelocity = 800.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	float SlamVelocity = -2000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	float SlamDamage = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	float SlamRadius = 300.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	float MinHeightForSlam = 200.0f;
	
	// Override to check if we can slam
	virtual bool CanExecute() const override;
	
protected:
	// Override from UtilityAbility
	virtual void ApplyMovement(ACharacter* Character) override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	// Track slam state
	bool bIsInSlamMode = false;
	bool bIsSlamming = false;
	float JumpStartHeight = 0.0f;
	
	// Handle landing impact
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);
	
	// Apply slam damage in area
	void ApplySlamDamage(const FVector& ImpactLocation);
	
	// Check if we're high enough to slam
	bool CanInitiateSlam() const;
};