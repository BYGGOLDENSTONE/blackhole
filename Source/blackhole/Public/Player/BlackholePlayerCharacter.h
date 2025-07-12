#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Interfaces/ResourceConsumer.h"
#include "Data/ComboDataAsset.h"
#include "BlackholePlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UIntegrityComponent;
class UWillPowerComponent;
class USlashAbilityComponent;
// class USystemFreezeAbilityComponent; // Removed
class UKillAbilityComponent;
class UGravityPullAbilityComponent;
class UHackerDashAbility;
class UHackerJumpAbility;
class UPulseHackAbility;
class UFirewallBreachAbility;
class UDataSpikeAbility;
class USystemOverrideAbility;
class UDashSlashCombo;
class UJumpSlashCombo;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
class UWallRunComponent;

UCLASS()
class BLACKHOLE_API ABlackholePlayerCharacter : public ACharacter, public IResourceConsumer
{
	GENERATED_BODY()

public:
	ABlackholePlayerCharacter();

	
	// Death handling
	UFUNCTION(BlueprintCallable, Category = "Character")
	void Die();
	
	UFUNCTION(BlueprintPure, Category = "Character")
	bool IsDead() const { return bIsDead; }
	
	// Update camera settings at runtime
	UFUNCTION(BlueprintCallable, Category = "Camera Settings")
	void UpdateCameraSettings();
	
	// Get what the crosshair is aiming at
	UFUNCTION(BlueprintCallable, Category = "Aiming")
	bool GetCrosshairTarget(FHitResult& OutHit, float MaxRange = 10000.0f) const;
	
	// Get aim direction and location from camera
	UFUNCTION(BlueprintCallable, Category = "Aiming")
	void GetAimLocationAndDirection(FVector& OutLocation, FVector& OutDirection) const;


protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComponent;
	
	// Camera Settings - Editable in Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (DisplayName = "Camera Distance"))
	float CameraArmLength = 300.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (DisplayName = "Camera Offset (Left/Right)"))
	float CameraOffsetY = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (DisplayName = "Camera Offset (Up/Down)"))
	float CameraOffsetZ = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (DisplayName = "Camera Target Height"))
	float CameraTargetOffsetZ = 60.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (DisplayName = "Camera Field of View"))
	float CameraFOV = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (DisplayName = "Enable Camera Lag"))
	bool bEnableCameraLag = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (DisplayName = "Camera Lag Speed"))
	float CameraLagSpeed = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (DisplayName = "Camera Rotation Lag Speed"))
	float CameraRotationLagSpeed = 12.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UIntegrityComponent* IntegrityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UWillPowerComponent* WillPowerComponent;

	// Movement Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UWallRunComponent* WallRunComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USlashAbilityComponent* SlashAbility;

	// System Freeze ability removed

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UKillAbilityComponent* KillAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UGravityPullAbilityComponent* GravityPullAbility;

	// Utility Abilities
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UHackerDashAbility* HackerDashAbility;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UHackerJumpAbility* HackerJumpAbility;


	// Path-specific Combat Abilities
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UPulseHackAbility* PulseHackAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UFirewallBreachAbility* FirewallBreachAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UDataSpikeAbility* DataSpikeAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	USystemOverrideAbility* SystemOverrideAbility;



	// Combo ability components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combos")
	UDashSlashCombo* DashSlashCombo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combos")
	UJumpSlashCombo* JumpSlashCombo;

	// Enhanced Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// Basic jump removed - use UtilityJumpAction for path-specific jumps
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	// UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	// Removed - Slash is now part of ability slots
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	// UInputAction* SlashAction;

	// System Freeze input removed

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* KillAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* GravityPullAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleCameraAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* UtilityJumpAction;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MenuToggleAction;  // Quote key for menu

	// Path-based ability slots (6 total)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AbilitySlot1Action;  // Left Mouse Button

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AbilitySlot2Action;  // Right Mouse Button

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AbilitySlot3Action;  // Q key

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AbilitySlot4Action;  // E key

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AbilitySlot5Action;  // R key

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AbilitySlot6Action;  // F key

	// Input functions
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// Core abilities (not slot-based)
	void UseKill();
	void UseGravityPull();
	void ToggleCamera();
	void UseDash();
	void UseUtilityJump();
	void ToggleMenu();
	
	// Path-based ability slots (6 total)
	void UseAbilitySlot1();  // LMB
	void UseAbilitySlot2();  // RMB
	void UseAbilitySlot3();  // Q
	void UseAbilitySlot4();  // E
	void UseAbilitySlot5();  // R
	void UseAbilitySlot6();  // F

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bIsFirstPerson;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FirstPersonCameraOffset;
	
	// Weapon mesh component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* KatanaWeaponMesh;
	
	// Head bone name for hiding in first person
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FirstPerson")
	FName HeadBoneName;
	
	// Weapon socket name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponSocketName;
	
private:
	// Hide/show head bone for first person
	void SetHeadVisibility(bool bVisible);
	
	
	// Update weapon visibility based on current path
	void UpdateWeaponVisibility();
	
	// Death state
	bool bIsDead = false;
	
	// Combo tracking (legacy - will be replaced by ComboDetectionSubsystem)
	enum class ELastAbilityUsed : uint8
	{
		None,
		Dash,
		Jump
	};
	
	ELastAbilityUsed LastAbilityUsed = ELastAbilityUsed::None;
	float LastAbilityTime = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (AllowPrivateAccess = "true"))
	float ComboWindowDuration = 0.5f; // 500ms window for combos
	
	// Combo data asset for new system
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (AllowPrivateAccess = "true"))
	UComboDataAsset* ComboDataAsset;
	
	// Handle ThresholdManager death event
	UFUNCTION()
	void OnThresholdDeath();
	

public:
	virtual void Tick(float DeltaTime) override;
	
	// Getter for camera component - used by abilities for camera-based aiming
	UFUNCTION(BlueprintCallable, Category = "Camera")
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	// Getter for wall run component - used by abilities and movement systems
	UFUNCTION(BlueprintCallable, Category = "Movement")
	UWallRunComponent* GetWallRunComponent() const { return WallRunComponent; }
	
	// IResourceConsumer interface implementation
	virtual bool HasResources_Implementation(float StaminaCost, float WPCost) const override;
	virtual bool ConsumeResources_Implementation(float StaminaCost, float WPCost) override;
	virtual void GetResourcePercentages_Implementation(float& OutStaminaPercent, float& OutWPPercent) const override;
};