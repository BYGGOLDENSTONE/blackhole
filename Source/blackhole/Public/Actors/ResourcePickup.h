#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourcePickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UResourceManager;

UENUM(BlueprintType)
enum class EPickupType : uint8
{
	WillPower		UMETA(DisplayName = "WillPower Pack"),
	HeatVent		UMETA(DisplayName = "Heat Vent")
};

UCLASS()
class BLACKHOLE_API AResourcePickup : public AActor
{
	GENERATED_BODY()
	
public:	
	AResourcePickup();

protected:
	virtual void BeginPlay() override;

	// Pickup type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	EPickupType PickupType;
	
	// Amount to restore
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float RestoreAmount = 20.0f;
	
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PickupMesh;
	
	// Respawn time (0 = no respawn)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float RespawnTime = 30.0f;
	
	// Handle overlap
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	// Apply pickup effect
	void ApplyPickupEffect(AActor* TargetActor);
	
	// Respawn pickup
	void RespawnPickup();
	
	// Timer handle for respawn
	FTimerHandle RespawnTimerHandle;
	
	// Resource manager reference
	UPROPERTY()
	UResourceManager* ResourceManager;
	
	// Visual effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* PickupEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* IdleEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* PickupSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float RotationSpeed = 90.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float BobSpeed = 2.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float BobHeight = 20.0f;
	
	// Active idle effect component
	UPROPERTY()
	UParticleSystemComponent* ActiveIdleEffect;
	
	// Initial Z position for bobbing
	float InitialZ;
	
public:
	virtual void Tick(float DeltaTime) override;
};