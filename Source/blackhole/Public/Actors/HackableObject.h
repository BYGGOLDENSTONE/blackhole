#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/Interaction/HackableComponent.h"
#include "HackableObject.generated.h"

UCLASS()
class BLACKHOLE_API AHackableObject : public AActor
{
    GENERATED_BODY()

public:
    AHackableObject();

protected:
    virtual void BeginPlay() override;

    // The main mesh component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* MeshComponent;

    // The hackable component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UHackableComponent* HackableComponent;

public:
    // Allow setting mesh in editor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable Object")
    class UStaticMesh* StaticMesh;

    // Physics settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    bool bStartWithPhysics = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float Mass = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float LinearDamping = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float AngularDamping = 0.0f;

    // Hackable settings exposed for easy editing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable Settings")
    bool bIsHackable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable Settings")
    float MaxHackDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable Settings")
    EHackableLaunchDirection LaunchDirection = EHackableLaunchDirection::Auto;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hackable Settings")
    class UMaterialInterface* HighlightMaterial;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};