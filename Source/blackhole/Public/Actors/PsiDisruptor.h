#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PsiDisruptor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UWillPowerComponent;

UCLASS()
class BLACKHOLE_API APsiDisruptor : public AActor
{
	GENERATED_BODY()

public:
	APsiDisruptor();

	// Disruptor stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Psi-Disruptor")
	float Health = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Psi-Disruptor")
	float MaxHealth = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Psi-Disruptor")
	float DisruptionRadius = 2000.0f;
	
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BaseMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CoreMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* DisruptionField;
	
	// Take damage
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		class AController* EventInstigator, AActor* DamageCauser) override;
		
	// Can be destroyed by gravity pull ultimate
	UFUNCTION(BlueprintCallable, Category = "Psi-Disruptor")
	void DestroyByUltimate();
	
	UFUNCTION(BlueprintPure, Category = "Psi-Disruptor")
	float GetHealthPercent() const { return Health / MaxHealth; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle DisruptionTickHandle;
	TArray<AActor*> AffectedPlayers;
	
	void ApplyDisruption();
	void RemoveDisruption();
	
	UFUNCTION()
	void OnDisruptionFieldBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
		
	UFUNCTION()
	void OnDisruptionFieldEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};