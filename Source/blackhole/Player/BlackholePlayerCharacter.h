#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BlackholePlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UIntegrityComponent;
class UStaminaComponent;
class UWillPowerComponent;
class USlashAbilityComponent;
class USystemFreezeAbilityComponent;
class UKillAbilityComponent;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;

UCLASS()
class BLACKHOLE_API ABlackholePlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABlackholePlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UIntegrityComponent* IntegrityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UStaminaComponent* StaminaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UWillPowerComponent* WillPowerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USlashAbilityComponent* SlashAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USystemFreezeAbilityComponent* SystemFreezeAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UKillAbilityComponent* KillAbility;

	// Enhanced Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SlashAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SystemFreezeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* KillAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleCameraAction;

	// Input functions
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void UseSlash();
	void UseSystemFreeze();
	void UseKill();
	void ToggleCamera();

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bIsFirstPerson;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FirstPersonCameraOffset;
	
	// Weapon mesh components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* MazeWeaponMesh;
	
	// Head bone name for hiding in first person
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FirstPerson")
	FName HeadBoneName;
	
private:
	// Hide/show head bone for first person
	void SetHeadVisibility(bool bVisible);

public:
	virtual void Tick(float DeltaTime) override;
	
	// Getter for camera component - used by abilities for camera-based aiming
	UFUNCTION(BlueprintCallable, Category = "Camera")
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }
};