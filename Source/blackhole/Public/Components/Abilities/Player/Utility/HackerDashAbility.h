#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/UtilityAbility.h"
#include "HackerDashAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UHackerDashAbility : public UUtilityAbility
{
	GENERATED_BODY()
	
public:
	UHackerDashAbility();
	
	// Dash-specific parameters
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashSpeed = 3000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDuration = 0.15f;
	
public:
	// Override Execute to register with combo system
	virtual void Execute() override;

protected:
	// Override from UtilityAbility
	virtual void ApplyMovement(ACharacter* Character) override;
	virtual void StopMovement() override;
	
private:
	// Store original friction to restore after dash
	float OriginalFriction = 0.0f;
};