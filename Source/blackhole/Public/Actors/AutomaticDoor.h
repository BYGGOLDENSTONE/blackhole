#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AutomaticDoor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EDoorState : uint8
{
    Closed      UMETA(DisplayName = "Closed"),
    Opening     UMETA(DisplayName = "Opening"),
    Open        UMETA(DisplayName = "Open"),
    Closing     UMETA(DisplayName = "Closing")
};

UCLASS()
class BLACKHOLE_API AAutomaticDoor : public AActor
{
    GENERATED_BODY()
    
public:    
    AAutomaticDoor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Door Mesh"))
    UStaticMeshComponent* DoorMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Proximity Trigger"))
    UBoxComponent* ProximityTrigger;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Inside Trigger"))
    UBoxComponent* InsideTrigger;
    
    // Door Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings", meta = (DisplayName = "Door Height"))
    float DoorHeight = 600.0f; // Doubled from 300 for taller doors
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings", meta = (DisplayName = "Move Speed"))
    float MoveSpeed = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings", meta = (DisplayName = "Proximity Range"))
    float ProximityRange = 400.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings", meta = (DisplayName = "Look At Threshold"), meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LookAtThreshold = 0.7f; // Dot product threshold (0.7 = ~45 degrees)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings", meta = (DisplayName = "Auto Close Delay"))
    float AutoCloseDelay = 2.0f;
    
    // State
    UPROPERTY(BlueprintReadOnly, Category = "Door State")
    EDoorState CurrentState = EDoorState::Closed;
    
    UPROPERTY(BlueprintReadOnly, Category = "Door State")
    bool bPlayerInside = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Door State")
    bool bPlayerNearby = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Door State")
    bool bPlayerLooking = false;

private:
    FVector ClosedPosition;
    FVector OpenPosition;
    float CurrentHeight = 0.0f;
    FTimerHandle AutoCloseTimer;
    
    UPROPERTY()
    class ABlackholePlayerCharacter* CachedPlayer;
    
    // Functions
    void UpdateDoorMovement(float DeltaTime);
    void CheckPlayerProximityAndLook();
    bool IsPlayerLookingAtDoor() const;
    void OpenDoor();
    void CloseDoor();
    void StartAutoCloseTimer();
    void CancelAutoCloseTimer();
    
    // Overlap events
    UFUNCTION()
    void OnProximityBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnProximityEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
    
    UFUNCTION()
    void OnInsideBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnInsideEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};