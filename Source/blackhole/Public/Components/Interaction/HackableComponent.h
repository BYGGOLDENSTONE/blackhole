#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HackableComponent.generated.h"

UENUM(BlueprintType)
enum class EHackableLaunchDirection : uint8
{
    Auto            UMETA(DisplayName = "Auto-Detect"),
    XAxis           UMETA(DisplayName = "X Axis Only"),
    YAxis           UMETA(DisplayName = "Y Axis Only"),
    ZAxis           UMETA(DisplayName = "Z Axis Only")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UHackableComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHackableComponent();

protected:
    virtual void BeginPlay() override;

public:
    // Whether this object can currently be hacked
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable")
    bool bIsHackable = true;

    // Maximum distance from which this object can be hacked
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable")
    float MaxHackDistance = 1000.0f;

    // Mass of the object (affects launch force)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable")
    float ObjectMass = 100.0f;

    // Predetermined launch direction
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable")
    EHackableLaunchDirection LaunchDirection = EHackableLaunchDirection::Auto;

    // Visual indicator when object is targeted
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable")
    class UMaterialInterface* HighlightMaterial = nullptr;

    // Original material to restore after hack
    UPROPERTY()
    class UMaterialInterface* OriginalMaterial = nullptr;

    // Check if object can be hacked
    bool CanBeHacked() const { return bIsHackable; }

    // Called when object is targeted for hacking
    void OnTargeted();

    // Called when object is no longer targeted
    void OnUntargeted();

    // Called when object is hacked and launched
    void OnHacked(float LaunchForce, const FVector& PlayerLocation);
    
    // Get the launch direction based on settings and player location
    FVector GetLaunchDirection(const FVector& PlayerLocation) const;

private:
    // Reference to the mesh component
    UPROPERTY()
    class UPrimitiveComponent* MeshComponent = nullptr;

    // Timer handle for resetting hackable state
    FTimerHandle ResetHackableTimer;

    // Reset the hackable state after being launched
    void ResetHackableState();
};