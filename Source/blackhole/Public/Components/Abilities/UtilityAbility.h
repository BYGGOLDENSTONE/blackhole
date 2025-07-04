#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "blackhole.h"
#include "UtilityAbility.generated.h"

class UCharacterMovementComponent;

UCLASS(Abstract, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UUtilityAbility : public UAbilityComponent
{
	GENERATED_BODY()
	
public:
	UUtilityAbility();
	
	// Path type for this utility
	UPROPERTY(EditDefaultsOnly, Category = "Utility")
	ECharacterPath PathType = ECharacterPath::None;
	
	// Movement parameters
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float MovementForce = 1000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float MovementDuration = 0.2f;
	
	// Override from AbilityComponent
	virtual void Execute() override;
	virtual bool CanExecute() const override;
	
protected:
	virtual void BeginPlay() override;
	
	// Override to implement path-specific behavior
	virtual void ApplyMovement(class ACharacter* Character);
	
	// Cached character movement component
	UPROPERTY()
	UCharacterMovementComponent* CachedMovement;
	
	// Timer for movement duration
	FTimerHandle MovementTimerHandle;
	
	// Helper to stop movement (called by timer)
	virtual void StopMovement();
	
	// Get the character owner
	class ACharacter* GetCharacterOwner() const;
};