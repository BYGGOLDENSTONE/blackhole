#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

class UIntegrityComponent;
class UStaticMeshComponent;
class ABlackholePlayerCharacter;
class UEnemyStateMachine;

UCLASS()
class BLACKHOLE_API ABaseEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseEnemy();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UIntegrityComponent* IntegrityComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UEnemyStateMachine* StateMachine;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual void UpdateAIBehavior(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual AActor* GetTargetActor() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual void SetTargetActor(AActor* NewTarget);

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	AActor* TargetActor;

	UFUNCTION()
	virtual void OnDeath();

	bool bIsDead;
	
	// Track if this enemy has started combat
	bool bHasStartedCombat = false;
	
	// Weapon mesh component - all enemies carry swords
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* SwordMesh;
	
	// Timer for AI updates
	FTimerHandle AIUpdateTimer;
	
	// AI update rate
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AIUpdateRate;
	
	// Timer-based AI update
	void TimerUpdateAI();
	
	// Store default walk speed
	float DefaultWalkSpeed;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// Utility accessors for EnemyUtility
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDead() const { return bIsDead; }
	
	UFUNCTION(BlueprintPure, Category = "Enemy")
	ABlackholePlayerCharacter* GetTargetPlayer() const;
	
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool HasStartedCombat() const { return bHasStartedCombat; }
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetCombatStarted(bool bStarted) { bHasStartedCombat = bStarted; }
	
	// Combat events
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnCombatStart();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnCombatEnd();
	
	// State machine integration
	UFUNCTION(BlueprintPure, Category = "Enemy")
	AActor* GetTarget() const { return TargetActor; }
	
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetDefaultWalkSpeed() const;
	
	// Movement modifiers
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ApplyMovementSpeedModifier(float Multiplier, float Duration);
	
	// Combat abilities
	UFUNCTION(BlueprintPure, Category = "Enemy")
	virtual bool CanBlock() const { return false; }
	
	UFUNCTION(BlueprintPure, Category = "Enemy")
	virtual bool CanDodge() const { return false; }
	
	UFUNCTION(BlueprintPure, Category = "Enemy")
	virtual bool HasPatrolRoute() const { return false; }
	
	// State reactions
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void PlayAlertReaction();
	
private:
	FTimerHandle SpeedResetTimerHandle;
};