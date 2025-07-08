#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

class UIntegrityComponent;
class UStaticMeshComponent;

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

public:
	virtual void Tick(float DeltaTime) override;
};