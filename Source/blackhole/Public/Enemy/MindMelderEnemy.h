#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "MindMelderEnemy.generated.h"

class UPowerfulMindmeldComponent;
class UDodgeComponent;

UCLASS()
class BLACKHOLE_API AMindMelderEnemy : public ABaseEnemy
{
	GENERATED_BODY()

public:
	AMindMelderEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void UpdateAIBehavior(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		class AController* EventInstigator, AActor* DamageCauser) override;
	
	// Override base enemy capabilities - can only dodge, no melee
	virtual bool CanBlock() const override { return false; }
	virtual bool CanDodge() const override { return true; }
	virtual void OnDeath() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UPowerfulMindmeldComponent* PowerfulMindmeld;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UDodgeComponent* DodgeAbility;

public:
	// Combat stats - public for data table access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "Mindmeld Range"))
	float MindmeldRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "Safe Distance"))
	float SafeDistance = 2000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "Retreat When Damaged"))
	bool bRetreatWhenDamaged = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "Dodge Chance", ClampMin = "0.0", ClampMax = "1.0"))
	float DodgeChance = 0.6f;
	
	// Get mindmeld component for state machine access
	UFUNCTION(BlueprintPure, Category = "Combat")
	UPowerfulMindmeldComponent* GetPowerfulMindmeld() const { return PowerfulMindmeld; }
	
	// Called when starting mindmeld
	UFUNCTION()
	void OnMindmeldStarted(float CastTime);
	
	UFUNCTION()
	void OnMindmeldComplete();
	
	UFUNCTION()
	void OnMindmeldInterrupted();

private:
	void MaintainSafeDistance(float DeltaTime);
	bool HasLineOfSightToTarget() const;
	float GetDistanceToTarget() const;
	void TryDodge();
	
	bool bIsChanneling;
	FTimerHandle RetreatTimer;
	
	void StopRetreating();
};