#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/UtilityAbility.h"
#include "ForgeDashAbility.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UForgeDashAbility : public UUtilityAbility
{
	GENERATED_BODY()
	
public:
	UForgeDashAbility();
	
	// Forge dash parameters
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float ChargeSpeed = 2000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float ChargeDuration = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float ImpactDamage = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float ImpactRadius = 150.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float StaggerDuration = 0.5f;
	
protected:
	// Override from UtilityAbility
	virtual void ApplyMovement(ACharacter* Character) override;
	virtual void StopMovement() override;
	
private:
	// Handle collision during charge
	UFUNCTION()
	void OnCharacterHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	// Check for enemies in impact radius
	void CheckImpactDamage();
	
	// Store original values
	float OriginalFriction = 0.0f;
	bool bIsCharging = false;
};