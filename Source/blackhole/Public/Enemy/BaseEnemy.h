#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

class UStaticMeshComponent;
class ABlackholePlayerCharacter;
class UEnemyStateMachine;
class UStatusEffectComponent;
class UGravityDirectionComponent;

UCLASS()
class BLACKHOLE_API ABaseEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseEnemy();
	
	// Override to handle damage as WP reduction
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		class AController* EventInstigator, AActor* DamageCauser) override;
	
	// Get state machine for ability components
	UFUNCTION(BlueprintPure, Category = "AI")
	UEnemyStateMachine* GetStateMachine() const { return StateMachine; }

protected:
	virtual void BeginPlay() override;

	// Status effect component for managing states
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UStatusEffectComponent* StatusEffectComponent;
	
	// Gravity direction component for handling gravity shifts
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UGravityDirectionComponent* GravityDirectionComponent;

	// Enemy health is now tracked as WP (same as player)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	float CurrentWP;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
	float MaxWP;
	
	// WP reward given to player on kill
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
	float WPRewardOnKill = 10.0f;
	
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
	// Combat configuration - public for state machine access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "Minimum Engagement Distance"))
	float MinimumEngagementDistance;
	
	// Data Table Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	class UDataTable* EnemyStatsDataTable;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FName StatsRowName;
	
	// Load stats from data table
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void LoadStatsFromDataTable();
	
	virtual void Tick(float DeltaTime) override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// Utility accessors for EnemyUtility
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDead() const { return bIsDead; }
	
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsAlive() const { return CurrentWP > 0.0f; }
	
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetCurrentWP() const { return CurrentWP; }
	
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetMaxWP() const { return MaxWP; }
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetCurrentWP(float NewWP) { CurrentWP = FMath::Clamp(NewWP, 0.0f, MaxWP); }
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetMaxWP(float NewMaxWP) { MaxWP = FMath::Max(0.0f, NewMaxWP); }
	
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
	
	// Stagger system (legacy - use StatusEffectComponent instead)
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat", meta = (DeprecatedFunction, DeprecationMessage = "Use StatusEffectComponent->ApplyStatusEffect instead"))
	void ApplyStagger(float Duration);
	
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat", meta = (DeprecatedFunction, DeprecationMessage = "Use StatusEffectComponent->IsStaggered instead"))
	bool IsStaggered() const;
	
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