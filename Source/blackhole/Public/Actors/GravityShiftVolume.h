#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/GravityDirectionComponent.h"
#include "GravityShiftVolume.generated.h"

UCLASS()
class BLACKHOLE_API AGravityShiftVolume : public AActor
{
    GENERATED_BODY()

public:
    AGravityShiftVolume();

protected:
    virtual void BeginPlay() override;

    // Overlap events
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
    // Trigger volume
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* TriggerVolume;

    // Visual indicator
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UArrowComponent* GravityDirectionArrow;

    // Gravity settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    EGravityAxis GravityAxis = EGravityAxis::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    FVector CustomGravityDirection = FVector(0, 0, -1);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    bool bUseCustomDirection = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    float TransitionDuration = 2.0f;

    // Should restore original gravity on exit?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    bool bRestoreOnExit = true;

    // Only affect players?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    bool bOnlyAffectPlayers = false;

private:
    // Track affected actors
    UPROPERTY()
    TMap<AActor*, UGravityDirectionComponent*> AffectedActors;

    // Apply gravity to actor
    void ApplyGravityToActor(AActor* Actor);

    // Restore gravity for actor
    void RestoreGravityForActor(AActor* Actor);
};