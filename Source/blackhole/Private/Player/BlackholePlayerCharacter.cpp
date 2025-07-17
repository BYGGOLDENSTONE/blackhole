#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Systems/GameStateManager.h"
#include "Systems/ComboDetectionSubsystem.h"
#include "UI/BlackholeHUD.h"
#include "Components/Abilities/Player/Basic/SlashAbilityComponent.h"
// #include "Components/Abilities/Player/SystemFreezeAbilityComponent.h" // Removed
#include "Components/Abilities/Player/Basic/KillAbilityComponent.h"
#include "Components/Abilities/Player/Hacker/GravityPullAbilityComponent.h"
#include "Components/Abilities/Player/Utility/HackerDashAbility.h"
#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "Components/Abilities/Player/Hacker/PulseHackAbility.h"
#include "Components/Abilities/Player/Hacker/FirewallBreachAbility.h"
#include "Components/Abilities/Player/Hacker/DataSpikeAbility.h"
#include "Components/Abilities/Player/Hacker/SystemOverrideAbility.h"
#include "Components/Abilities/ComboAbilityComponent.h"
#include "Components/Abilities/Combos/DashSlashCombo.h"
#include "Components/Abilities/Combos/JumpSlashCombo.h"
#include "Components/Abilities/Combos/DashWallRunCombo.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Components/Movement/WallRunComponent.h"
#include "Components/StatusEffectComponent.h"
#include "Config/GameplayConfig.h"
#include "Engine/World.h"

ABlackholePlayerCharacter::ABlackholePlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(GameplayConfig::Movement::CAPSULE_RADIUS, GameplayConfig::Movement::CAPSULE_HEIGHT);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, GameplayConfig::Movement::ROTATION_RATE, 0.0f);
	GetCharacterMovement()->JumpZVelocity = GameplayConfig::Movement::BASE_JUMP_VELOCITY; // Base jump velocity for movement component
	GetCharacterMovement()->AirControl = GameplayConfig::Movement::AIR_CONTROL;
	
	// Momentum preservation settings
	GetCharacterMovement()->GroundFriction = GroundFriction; // Use editable property
	GetCharacterMovement()->BrakingDecelerationWalking = BrakingDeceleration; // Use editable property
	GetCharacterMovement()->BrakingDecelerationFalling = 0.0f; // No air deceleration for momentum preservation
	GetCharacterMovement()->BrakingDecelerationFlying = 0.0f;
	GetCharacterMovement()->BrakingDecelerationSwimming = 0.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 0.5f; // Reduced from default 2.0f
	GetCharacterMovement()->bUseSeparateBrakingFriction = false; // Use same friction for all states
	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed; // Use editable property
	
	// Additional momentum settings
	GetCharacterMovement()->MaxAcceleration = 1200.0f; // Reduced from default 2048.0f for smoother acceleration
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f; // Lower threshold for analog movement

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = CameraArmLength; // Use editable property
	SpringArmComponent->bUsePawnControlRotation = true;
	
	// Offset camera to the left and up for over-the-shoulder view
	SpringArmComponent->SocketOffset = FVector(0.0f, CameraOffsetY, CameraOffsetZ); // Use editable properties
	SpringArmComponent->TargetOffset = FVector(0.0f, 0.0f, CameraTargetOffsetZ); // Use editable property
	
	// Enable camera lag for smoother movement
	SpringArmComponent->bEnableCameraLag = bEnableCameraLag;
	SpringArmComponent->CameraLagSpeed = CameraLagSpeed;
	SpringArmComponent->bEnableCameraRotationLag = bEnableCameraLag;
	SpringArmComponent->CameraRotationLagSpeed = CameraRotationLagSpeed;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;
	
	// Set FOV from editable property
	CameraComponent->SetFieldOfView(CameraFOV);

	WillPowerComponent = CreateDefaultSubobject<UWillPowerComponent>(TEXT("WillPower"));
	StatusEffectComponent = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusEffectComponent"));

	// Create movement components
	WallRunComponent = CreateDefaultSubobject<UWallRunComponent>(TEXT("WallRunComponent"));

	SlashAbility = CreateDefaultSubobject<USlashAbilityComponent>(TEXT("SlashAbility"));
	// SystemFreezeAbility removed
	KillAbility = CreateDefaultSubobject<UKillAbilityComponent>(TEXT("KillAbility"));
	GravityPullAbility = CreateDefaultSubobject<UGravityPullAbilityComponent>(TEXT("GravityPullAbility"));

	// Create utility abilities
	HackerDashAbility = CreateDefaultSubobject<UHackerDashAbility>(TEXT("HackerDashAbility"));
	HackerJumpAbility = CreateDefaultSubobject<UHackerJumpAbility>(TEXT("HackerJumpAbility"));

	// Create Hacker abilities
	PulseHackAbility = CreateDefaultSubobject<UPulseHackAbility>(TEXT("PulseHackAbility"));
	FirewallBreachAbility = CreateDefaultSubobject<UFirewallBreachAbility>(TEXT("FirewallBreachAbility"));
	DataSpikeAbility = CreateDefaultSubobject<UDataSpikeAbility>(TEXT("DataSpikeAbility"));
	SystemOverrideAbility = CreateDefaultSubobject<USystemOverrideAbility>(TEXT("SystemOverrideAbility"));
	
	// Create combo ability components
	DashSlashCombo = CreateDefaultSubobject<UDashSlashCombo>(TEXT("DashSlashCombo"));
	JumpSlashCombo = CreateDefaultSubobject<UJumpSlashCombo>(TEXT("JumpSlashCombo"));
	DashWallRunCombo = CreateDefaultSubobject<UDashWallRunCombo>(TEXT("DashWallRunCombo"));

	bIsFirstPerson = false;
	FirstPersonCameraOffset = GameplayConfig::Movement::FIRST_PERSON_OFFSET;
	HeadBoneName = "head"; // Default head bone name - adjust in Blueprint if different
	WeaponSocketName = "weaponsocket"; // Default socket name - adjust in Blueprint if different
	
	// Create katana weapon mesh component
	KatanaWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaWeapon"));
	KatanaWeaponMesh->SetupAttachment(GetMesh());
	KatanaWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	KatanaWeaponMesh->SetCastShadow(true);
}

void ABlackholePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Reset death state on BeginPlay - fixes crash when restarting after death
	bIsDead = false;

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set Hacker path in ResourceManager
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			ResourceMgr->SetCurrentPath(ECharacterPath::Hacker);
		}
	}
	
	// Attach weapon to the specified socket
	if (GetMesh() && !WeaponSocketName.IsNone())
	{
		if (KatanaWeaponMesh)
		{
			KatanaWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
			KatanaWeaponMesh->SetVisibility(true);
		}
	}
	
	// Bind to ThresholdManager death event
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			ThresholdMgr->OnPlayerDeath.AddDynamic(this, &ABlackholePlayerCharacter::OnThresholdDeath);
		}
	}
	
	// Bind to ResourceManager's OnWPDepleted event (WP reached 0 activates ultimates)
	if (UResourceManager* ResourceMgr = GetGameInstance()->GetSubsystem<UResourceManager>())
	{
		ResourceMgr->OnWPDepleted.AddDynamic(this, &ABlackholePlayerCharacter::OnWPDepleted);
	}
	
	// Initialize movement speed to walk speed
	bIsRunning = false;
	SetMovementSpeed(false);
	
}

void ABlackholePlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Reset camera to third person on cleanup to prevent crashes
	if (bIsFirstPerson && IsValid(CameraComponent) && IsValid(SpringArmComponent))
	{
		bIsFirstPerson = false;
		
		// Safely reattach camera to spring arm for cleanup
		CameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform, USpringArmComponent::SocketName);
		CameraComponent->SetRelativeLocation(FVector::ZeroVector);
		CameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
		CameraComponent->bUsePawnControlRotation = false;
		
		// Re-enable spring arm
		SpringArmComponent->bUsePawnControlRotation = true;
		SpringArmComponent->bInheritPitch = true;
		SpringArmComponent->bInheritYaw = true;
		SpringArmComponent->bInheritRoll = true;
		
		// Show head bone again for cleanup
		SetHeadVisibility(true);
	}
	
	// Unbind from all events to prevent dangling references
	
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			ThresholdMgr->OnPlayerDeath.RemoveDynamic(this, &ABlackholePlayerCharacter::OnThresholdDeath);
		}
	}
	
	if (UResourceManager* ResourceMgr = GetGameInstance()->GetSubsystem<UResourceManager>())
	{
		ResourceMgr->OnWPDepleted.RemoveDynamic(this, &ABlackholePlayerCharacter::OnWPDepleted);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ABlackholePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Death is now handled by event from IntegrityComponent
	
}

bool ABlackholePlayerCharacter::HasResources_Implementation(float StaminaCost, float WPCost) const
{
	// StaminaCost parameter kept for interface compatibility but ignored
	
	// Check WP status
	if (UResourceManager* ResourceManager = GetGameInstance()->GetSubsystem<UResourceManager>())
	{
		float CurrentWP = ResourceManager->GetCurrentWillPower();
		
		// If WP > 0, always allow ability use (to trigger critical state)
		if (CurrentWP > 0.0f)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("HasResources: WP > 0 (%.1f) - allowing ability use"), CurrentWP);
			return true;
		}
		
		// If WP = 0, block (ultimate mode will bypass this)
		if (CurrentWP <= 0.0f)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("HasResources: WP at 0 - blocking unless in ultimate mode"));
			return false;
		}
	}
	
	return true;
}

bool ABlackholePlayerCharacter::ConsumeResources_Implementation(float StaminaCost, float WPCost)
{
	bool bSuccess = true;
	
	// StaminaCost parameter kept for interface compatibility but ignored
	
	// Consume WP (energy)
	if (WPCost > 0.0f)
	{
		if (UResourceManager* ResourceManager = GetGameInstance()->GetSubsystem<UResourceManager>())
		{
			if (!ResourceManager->ConsumeWillPower(WPCost))
			{
				UE_LOG(LogTemp, Warning, TEXT("Player: Not enough WP to use ability (need %.1f)"), WPCost);
				return false;
			}
		}
	}
	
	return bSuccess;
}

void ABlackholePlayerCharacter::GetResourcePercentages_Implementation(float& OutStaminaPercent, float& OutWPPercent) const
{
	// Stamina no longer exists - always return 0
	OutStaminaPercent = 0.0f;
	
	// Get WP percentage
	if (UResourceManager* ResourceManager = GetGameInstance()->GetSubsystem<UResourceManager>())
	{
		OutWPPercent = ResourceManager->GetWillPowerPercent();
	}
	else
	{
		OutWPPercent = 0.0f;
	}
}

void ABlackholePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping is handled by utility abilities (UseUtilityJump)
		// Basic jump removed to avoid conflicts with path-specific jumps

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::Look);

		// Core abilities (shared between paths)
		// SystemFreeze removed
		EnhancedInputComponent->BindAction(KillAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseKill);

		// Camera toggle
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::ToggleCamera);
		
		// Menu toggle
		if (MenuToggleAction)
		{
			EnhancedInputComponent->BindAction(MenuToggleAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::ToggleMenu);
		}
		
		// Utility abilities
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseDash);
		EnhancedInputComponent->BindAction(UtilityJumpAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseUtilityJump);
		
		
		
		// Path-based ability slots (6 total)
		EnhancedInputComponent->BindAction(AbilitySlot1Action, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseAbilitySlot1);
		EnhancedInputComponent->BindAction(AbilitySlot2Action, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseAbilitySlot2);
		EnhancedInputComponent->BindAction(AbilitySlot3Action, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseAbilitySlot3);
		EnhancedInputComponent->BindAction(AbilitySlot4Action, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseAbilitySlot4);
		EnhancedInputComponent->BindAction(AbilitySlot5Action, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseAbilitySlot5);
		EnhancedInputComponent->BindAction(AbilitySlot6Action, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseAbilitySlot6);
	}
}

void ABlackholePlayerCharacter::Move(const FInputActionValue& Value)
{
	// Check if status effects allow movement
	if (StatusEffectComponent && !StatusEffectComponent->CanMove())
	{
		return; // Movement blocked by status effects
	}
	
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	
	// Check for W key press (forward movement)
	if (MovementVector.Y > 0.0f && !bWKeyPressed)
	{
		bWKeyPressed = true;
		HandleWKeyPress();
	}
	else if (MovementVector.Y <= 0.0f && bWKeyPressed)
	{
		bWKeyPressed = false;
	}

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
		
		// Note: Wall run component will override velocity when active
		// We still need to process input so HasForwardInput() works correctly
	}
}

void ABlackholePlayerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// Slash is now handled through ability slots
// void ABlackholePlayerCharacter::UseSlash()
// {
// 	if (SlashAbility)
// 	{
// 		SlashAbility->Execute();
// 		if (ComboTracker)
// 		{
// 			ComboTracker->RegisterAbilityUse(SlashAbility);
// 		}
// 	}
// }

// SystemFreeze removed

void ABlackholePlayerCharacter::UseKill()
{
	if (KillAbility && KillAbility->CanExecute())
	{
		KillAbility->Execute();
	}
}


void ABlackholePlayerCharacter::ToggleCamera()
{
	// Safety checks - don't switch camera if character is invalid or dead
	if (!IsValid(this) || bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot toggle camera - character is invalid or dead"));
		return;
	}
	
	// Check if all required components are valid
	if (!IsValid(CameraComponent) || !IsValid(SpringArmComponent) || !IsValid(GetMesh()) || !IsValid(GetCharacterMovement()))
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerCharacter: Cannot toggle camera - required components are invalid"));
		return;
	}
	
	// Don't allow camera switching during ability execution or invalid movement states
	if (IsValid(GetCharacterMovement()))
	{
		EMovementMode CurrentMovementMode = GetCharacterMovement()->MovementMode;
		if (CurrentMovementMode != MOVE_Walking && CurrentMovementMode != MOVE_Falling)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot toggle camera during movement mode %d - wait until walking or falling"), (int32)CurrentMovementMode);
			return;
		}
	}
	
	bIsFirstPerson = !bIsFirstPerson;
	
	if (bIsFirstPerson)
	{
		// Switch to first person
		// Detach camera from spring arm and attach directly to mesh
		if (IsValid(CameraComponent) && IsValid(SpringArmComponent))
		{
			CameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		}
		
		// Attach camera to head socket if it exists, otherwise use relative position
		FName HeadSocket = "camerasocket"; // Your custom socket name
		if (IsValid(GetMesh()) && GetMesh()->DoesSocketExist(HeadSocket))
		{
			CameraComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HeadSocket);
			// Position camera slightly forward and up from head socket to avoid clipping
			CameraComponent->SetRelativeLocation(FVector(GameplayConfig::Movement::FP_CAMERA_FORWARD, 0.0f, GameplayConfig::Movement::FP_CAMERA_UP));
			CameraComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
		else
		{
			// Fallback: attach to mesh at approximate head height
			if (IsValid(GetMesh()) && IsValid(CameraComponent))
			{
				CameraComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
				// Adjust these values based on your character model
				CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, GameplayConfig::Movement::FP_FALLBACK_HEIGHT)); // Approximate head height
				CameraComponent->SetRelativeRotation(FRotator(GameplayConfig::Movement::FP_CAMERA_PITCH, GameplayConfig::Movement::FP_CAMERA_YAW, 0.0f)); // Look slightly down, face forward
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("PlayerCharacter: Cannot attach camera - mesh or camera component is invalid"));
				return;
			}
		}
		
		// Make camera use controller rotation directly
		if (IsValid(CameraComponent))
		{
			CameraComponent->bUsePawnControlRotation = true;
		}
		
		// Disable spring arm influence
		if (IsValid(SpringArmComponent))
		{
			SpringArmComponent->bUsePawnControlRotation = false;
			SpringArmComponent->bInheritPitch = false;
			SpringArmComponent->bInheritYaw = false;
			SpringArmComponent->bInheritRoll = false;
		}
		
		// Don't hide character mesh in first person - we want to see arms/legs
		if (IsValid(GetMesh()))
		{
			GetMesh()->SetOwnerNoSee(false);
		}
		
		// Hide the head bone to avoid seeing inside it
		SetHeadVisibility(false);
		
		// Adjust movement to feel better in first person
		bUseControllerRotationYaw = true;
		if (IsValid(GetCharacterMovement()))
		{
			GetCharacterMovement()->bOrientRotationToMovement = false;
		}
	}
	else
	{
		// Switch to third person
		// Reattach camera to spring arm
		if (IsValid(CameraComponent) && IsValid(SpringArmComponent))
		{
			CameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
			CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform, USpringArmComponent::SocketName);
			CameraComponent->SetRelativeLocation(FVector::ZeroVector);
			CameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
			CameraComponent->bUsePawnControlRotation = false;
			
			// Restore FOV from editable property
			CameraComponent->SetFieldOfView(CameraFOV);
		}
		
		// Re-enable spring arm with editable settings
		if (IsValid(SpringArmComponent))
		{
			SpringArmComponent->bUsePawnControlRotation = true;
			SpringArmComponent->bInheritPitch = true;
			SpringArmComponent->bInheritYaw = true;
			SpringArmComponent->bInheritRoll = true;
			SpringArmComponent->TargetArmLength = CameraArmLength; // Use editable property
			SpringArmComponent->SocketOffset = FVector(0.0f, CameraOffsetY, CameraOffsetZ); // Use editable properties
			SpringArmComponent->TargetOffset = FVector(0.0f, 0.0f, CameraTargetOffsetZ); // Use editable property
			
			// Re-enable camera lag with editable settings
			SpringArmComponent->bEnableCameraLag = bEnableCameraLag;
			SpringArmComponent->CameraLagSpeed = CameraLagSpeed;
			SpringArmComponent->bEnableCameraRotationLag = bEnableCameraLag;
			SpringArmComponent->CameraRotationLagSpeed = CameraRotationLagSpeed;
		}
		
		// Ensure character mesh is visible in third person
		if (IsValid(GetMesh()))
		{
			GetMesh()->SetOwnerNoSee(false);
		}
		
		// Show the head bone again
		SetHeadVisibility(true);
		
		// Reset movement settings
		bUseControllerRotationYaw = false;
		if (IsValid(GetCharacterMovement()))
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
		}
	}
	
}

void ABlackholePlayerCharacter::SetHeadVisibility(bool bVisible)
{
	// Safety checks - don't manipulate bones if character is invalid or dead
	if (!IsValid(this) || bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot set head visibility - character is invalid or dead"));
		return;
	}
	
	// Check if mesh is valid and head bone name is set
	if (!IsValid(GetMesh()) || HeadBoneName.IsNone() || HeadBoneName.ToString().IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot set head visibility - invalid mesh or head bone name"));
		return;
	}
	
	// Additional safety check - ensure the mesh has the bone
	if (!GetMesh()->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot set head visibility - no skeletal mesh asset"));
		return;
	}
	
	// Hide/show the head bone and all its children
	GetMesh()->HideBoneByName(HeadBoneName, bVisible ? EPhysBodyOp::PBO_None : EPhysBodyOp::PBO_Term);
}


void ABlackholePlayerCharacter::UseDash()
{
	// Safety check
	if (!IsValid(this) || bIsDead)
	{
		return;
	}
	
	// Check if status effects allow movement/dashing
	if (StatusEffectComponent && !StatusEffectComponent->CanMove())
	{
		return; // Movement/dashing blocked by status effects
	}
	
	// Only register input if ability can execute
	if (IsValid(HackerDashAbility) && HackerDashAbility->CanExecute())
	{
		// Execute the dash ability
		HackerDashAbility->Execute();
		
		// Legacy combo tracking
		LastAbilityUsed = ELastAbilityUsed::Dash;
		LastAbilityTime = GetWorld()->GetTimeSeconds();
		
		// New combo system
		if (UComboDetectionSubsystem* ComboSystem = GetWorld()->GetSubsystem<UComboDetectionSubsystem>())
		{
			ComboSystem->RegisterInput(this, EComboInput::Dash, GetActorLocation());
		}
	}
}

void ABlackholePlayerCharacter::UseUtilityJump()
{
	// Safety check
	if (!IsValid(this) || bIsDead)
	{
		return;
	}
	
	// Check if status effects allow movement/jumping
	if (StatusEffectComponent && !StatusEffectComponent->CanMove())
	{
		return; // Movement/jumping blocked by status effects
	}
	
	// Debug: Show that jump input was received
	UE_LOG(LogTemp, Warning, TEXT("UseUtilityJump() called!"));
	// Debug message removed - space pressed
	
	// Check wall running first - wall run takes priority for jump input
	if (IsValid(WallRunComponent))
	{
		bool bIsWallRunning = WallRunComponent->IsWallRunning();
		UE_LOG(LogTemp, Warning, TEXT("UseUtilityJump: WallRunComponent valid, IsWallRunning = %s"), 
			bIsWallRunning ? TEXT("TRUE") : TEXT("FALSE"));
		
		if (bIsWallRunning)
		{
			// During wall run, use hacker jump ability with directional input
			// Get player input for direction
			FVector InputVector = GetLastMovementInputVector();
			FVector RightVector = GetActorRightVector();
			float RightInput = FVector::DotProduct(InputVector, RightVector);
			
			// End wall run
			WallRunComponent->EndWallRun(true); // End with jump flag
			
			// Apply additional directional velocity based on A-D input
			if (FMath::Abs(RightInput) > 0.1f)
			{
				// Add horizontal velocity based on input direction
				if (UCharacterMovementComponent* Movement = GetCharacterMovement())
				{
					FVector AdditionalVelocity = RightVector * RightInput * 500.0f;
					Movement->AddImpulse(AdditionalVelocity, true);
				}
			}
			
			// Don't return - let the hacker jump ability execute below
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UseUtilityJump: WallRunComponent is NULL or invalid!"));
	}
	
	// Check if wall run can be started
	if (IsValid(WallRunComponent) && WallRunComponent->CanStartWallRun())
	{
		// Try to start wall run
		if (WallRunComponent->TryStartWallRun())
		{
			return; // Wall run started successfully
		}
	}
	
	// Normal jump logic - only execute if ability can execute
	// Note: HackerJumpAbility is allowed during wall run, so this will work
	if (IsValid(HackerJumpAbility) && HackerJumpAbility->CanExecute())
	{
		// Execute the jump ability
		HackerJumpAbility->Execute();
		
		// Legacy combo tracking
		LastAbilityUsed = ELastAbilityUsed::Jump;
		LastAbilityTime = GetWorld()->GetTimeSeconds();
		
		// New combo system
		if (UComboDetectionSubsystem* ComboSystem = GetWorld()->GetSubsystem<UComboDetectionSubsystem>())
		{
			ComboSystem->RegisterInput(this, EComboInput::Jump, GetActorLocation());
		}
	}
}


void ABlackholePlayerCharacter::ToggleMenu()
{
	// Get the HUD and tell it to handle menu toggle
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ABlackholeHUD* GameHUD = Cast<ABlackholeHUD>(PC->GetHUD()))
		{
			GameHUD->OnMenuTogglePressed();
		}
	}
}



void ABlackholePlayerCharacter::UseAbilitySlot1()
{
	// Safety check
	if (!IsValid(this) || bIsDead)
	{
		return;
	}
	
	// Left Mouse Button - Basic attack
	// Currently: Slash (shared) | Future: Katana Slash (Hacker) / Forge Slam (Forge)
	// For now, use the shared Slash ability for both paths
	
	// Check if we're in a valid movement state for slash
	bool bCanSlash = true;
	if (IsValid(GetCharacterMovement()))
	{
		// Don't allow slash during dash or other non-walking movement states
		if (GetCharacterMovement()->MovementMode != MOVE_Walking && 
			GetCharacterMovement()->MovementMode != MOVE_Falling)
		{
			bCanSlash = false;
			UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot slash during movement mode %d"), 
				(int32)GetCharacterMovement()->MovementMode);
		}
	}
	
	// Only register input if ability can execute and we're in a valid state
	if (bCanSlash && IsValid(SlashAbility) && SlashAbility->CanExecute())
	{
		// Register slash input for new combo system
		if (UComboDetectionSubsystem* ComboSystem = GetWorld()->GetSubsystem<UComboDetectionSubsystem>())
		{
			ComboSystem->RegisterInput(this, EComboInput::Slash, GetActorLocation());
		}
		
		// Legacy combo check (will be removed once new system is fully integrated)
		float CurrentTime = GetWorld()->GetTimeSeconds();
		float TimeSinceLastAbility = CurrentTime - LastAbilityTime;
		
		// Check if we're within the combo window
		if (TimeSinceLastAbility <= ComboWindowDuration)
		{
			// Check which combo to execute
			if (LastAbilityUsed == ELastAbilityUsed::Dash && IsValid(DashSlashCombo) && DashSlashCombo->CanExecute())
			{
				// Execute Dash + Slash combo (Phantom Strike)
				UE_LOG(LogTemp, Log, TEXT("Executing DashSlashCombo (Phantom Strike)"));
				DashSlashCombo->Execute();
				
				// Reset combo tracking
				LastAbilityUsed = ELastAbilityUsed::None;
				LastAbilityTime = 0.0f;
				return; // Don't execute normal slash
			}
			else if (LastAbilityUsed == ELastAbilityUsed::Jump && IsValid(JumpSlashCombo) && JumpSlashCombo->CanExecute())
			{
				// Execute Jump + Slash combo (Aerial Rave)
				UE_LOG(LogTemp, Log, TEXT("Executing JumpSlashCombo (Aerial Rave)"));
				JumpSlashCombo->Execute();
				
				// Reset combo tracking
				LastAbilityUsed = ELastAbilityUsed::None;
				LastAbilityTime = 0.0f;
				return; // Don't execute normal slash
			}
		}
		
		// No combo detected or combo window expired - execute normal slash
		SlashAbility->Execute();
		
		// Reset combo tracking since we used slash
		LastAbilityUsed = ELastAbilityUsed::None;
		LastAbilityTime = 0.0f;
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot2()
{
	// Right Mouse Button - Firewall Breach
	if (IsValid(FirewallBreachAbility) && FirewallBreachAbility->CanExecute())
	{
		FirewallBreachAbility->Execute();
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot3()
{
	// Q key - Pulse Hack
	if (IsValid(PulseHackAbility) && PulseHackAbility->CanExecute())
	{
		PulseHackAbility->Execute();
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot4()
{
	// E key - Gravity Pull
	if (IsValid(GravityPullAbility) && GravityPullAbility->CanExecute())
	{
		GravityPullAbility->Execute();
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot5()
{
	// R key - Data Spike
	if (IsValid(DataSpikeAbility) && DataSpikeAbility->CanExecute())
	{
		DataSpikeAbility->Execute();
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot6()
{
	// F key - System Override
	if (IsValid(SystemOverrideAbility) && SystemOverrideAbility->CanExecute())
	{
		SystemOverrideAbility->Execute();
	}
}


void ABlackholePlayerCharacter::UpdateCameraSettings()
{
	if (IsValid(SpringArmComponent))
	{
		SpringArmComponent->TargetArmLength = CameraArmLength;
		SpringArmComponent->SocketOffset = FVector(0.0f, CameraOffsetY, CameraOffsetZ);
		SpringArmComponent->TargetOffset = FVector(0.0f, 0.0f, CameraTargetOffsetZ);
		SpringArmComponent->bEnableCameraLag = bEnableCameraLag;
		SpringArmComponent->CameraLagSpeed = CameraLagSpeed;
		SpringArmComponent->bEnableCameraRotationLag = bEnableCameraLag;
		SpringArmComponent->CameraRotationLagSpeed = CameraRotationLagSpeed;
	}
	
	if (IsValid(CameraComponent))
	{
		CameraComponent->SetFieldOfView(CameraFOV);
	}
}

void ABlackholePlayerCharacter::UpdateMovementSettings()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GroundFriction = GroundFriction;
		Movement->BrakingDecelerationWalking = BrakingDeceleration;
		Movement->MaxWalkSpeed = MaxWalkSpeed;
		
		// Apply momentum preservation settings
		Movement->BrakingDecelerationFalling = 0.0f;
		Movement->BrakingDecelerationFlying = 0.0f;
		Movement->BrakingDecelerationSwimming = 0.0f;
		Movement->BrakingFrictionFactor = 0.5f;
		Movement->bUseSeparateBrakingFriction = false;
		Movement->MaxAcceleration = 1200.0f;
		Movement->MinAnalogWalkSpeed = 20.0f;
		
		UE_LOG(LogTemp, Warning, TEXT("Movement settings updated - Friction: %.2f, Deceleration: %.2f, MaxSpeed: %.2f"), 
			GroundFriction, BrakingDeceleration, MaxWalkSpeed);
	}
}

bool ABlackholePlayerCharacter::GetCrosshairTarget(FHitResult& OutHit, float MaxRange) const
{
	if (!IsValid(CameraComponent))
	{
		return false;
	}
	
	FVector CameraLocation = CameraComponent->GetComponentLocation();
	FVector CameraForward = CameraComponent->GetForwardVector();
	FVector TraceEnd = CameraLocation + (CameraForward * MaxRange);
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	
	// Trace from camera through crosshair
	return GetWorld()->LineTraceSingleByChannel(
		OutHit,
		CameraLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);
}

void ABlackholePlayerCharacter::GetAimLocationAndDirection(FVector& OutLocation, FVector& OutDirection) const
{
	if (IsValid(CameraComponent))
	{
		OutLocation = CameraComponent->GetComponentLocation();
		OutDirection = CameraComponent->GetForwardVector();
	}
	else
	{
		// Fallback to character location
		OutLocation = GetActorLocation();
		OutDirection = GetActorForwardVector();
	}
}

void ABlackholePlayerCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}
	
	bIsDead = true;
	
	UE_LOG(LogTemp, Error, TEXT("=== PLAYER DEATH TRIGGERED ==="));
	
	// Check if critical timer is active - we should NOT interfere with it
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			if (ThresholdMgr->IsCriticalTimerActive())
			{
				UE_LOG(LogTemp, Error, TEXT("WARNING: Player died while critical timer active! Timer should continue running."));
			}
		}
	}
	
	// IMPORTANT: Don't clear critical timer - let ThresholdManager handle it
	// We need to be more selective about which timers to clear to avoid interfering
	// with critical game systems like the critical timer
	// Clear only player-specific timers, not system timers
	
	// Unbind from all delegates BEFORE disabling components
	
	if (UResourceManager* ResourceMgr = GetGameInstance()->GetSubsystem<UResourceManager>())
	{
		ResourceMgr->OnWPDepleted.RemoveDynamic(this, &ABlackholePlayerCharacter::OnWPDepleted);
	}
	
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			ThresholdMgr->OnPlayerDeath.RemoveDynamic(this, &ABlackholePlayerCharacter::OnThresholdDeath);
		}
	}
	
	// Reset camera to third person on death to prevent FP crashes
	if (bIsFirstPerson && IsValid(CameraComponent) && IsValid(SpringArmComponent))
	{
		bIsFirstPerson = false; // Set flag first to prevent ToggleCamera from being called
		
		// Safely reattach camera to spring arm
		CameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform, USpringArmComponent::SocketName);
		CameraComponent->SetRelativeLocation(FVector::ZeroVector);
		CameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
		CameraComponent->bUsePawnControlRotation = false;
		
		// Re-enable spring arm
		SpringArmComponent->bUsePawnControlRotation = true;
		SpringArmComponent->bInheritPitch = true;
		SpringArmComponent->bInheritYaw = true;
		SpringArmComponent->bInheritRoll = true;
		
		// Show head bone again
		SetHeadVisibility(true);
	}
	
	// Disable all abilities safely
	TArray<UActorComponent*> AllComponents = GetComponents().Array();
	for (UActorComponent* Component : AllComponents)
	{
		if (IsValid(Component) && Component->IsA<UAbilityComponent>())
		{
			if (UAbilityComponent* Ability = Cast<UAbilityComponent>(Component))
			{
				// Don't clear ability timers - might interfere with critical timer system
				// Just disable component ticking
				Ability->SetComponentTickEnabled(false);
			}
		}
	}
	
	// Disable movement
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	
	// Stop character movement
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->DisableMovement();
		MovementComp->StopMovementImmediately();
	}
	
	// Play death animation/effects here
	// For now, just make the character fall
	if (IsValid(GetMesh()))
	{
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	
	// Show death message
	if (GEngine)
	{
		// Debug messages removed - death notification
	}
	
	// Notify GameStateManager
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameStateManager* GameStateMgr = GameInstance->GetSubsystem<UGameStateManager>())
		{
			GameStateMgr->EndGame(true);
		}
	}
}

// Removed CheckIntegrity - now using event-driven system

void ABlackholePlayerCharacter::OnThresholdDeath()
{
	UE_LOG(LogTemp, Error, TEXT("Player death triggered by ThresholdManager!"));
	Die();
}

void ABlackholePlayerCharacter::OnWPDepleted()
{
	UE_LOG(LogTemp, Warning, TEXT("Player: WP reached 0 - Starting critical timer!"));
	
	// Start critical timer and activate ultimate mode
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			// Activate ultimate mode first
			ThresholdMgr->ActivateUltimateMode();
			// Then start the critical timer
			ThresholdMgr->StartCriticalTimer();
		}
	}
}

float ABlackholePlayerCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
	class AController* EventInstigator, AActor* DamageCauser)
{
	// Check if critical timer is active or WP is at 0 - player is immune during this time
	if (UResourceManager* ResourceMgr = GetGameInstance()->GetSubsystem<UResourceManager>())
	{
		// If WP is at 0 (critical state), no damage
		if (ResourceMgr->GetCurrentWillPower() <= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player immune to damage - WP at 0 (critical state)!"));
			return 0.0f;
		}
		
		// Also check critical timer for safety
		if (UWorld* World = GetWorld())
		{
			if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
			{
				if (ThresholdMgr->IsCriticalTimerActive())
				{
					UE_LOG(LogTemp, Warning, TEXT("Player immune to damage during critical timer!"));
					return 0.0f;
				}
			}
		}
		
		// Route damage to WP
		ResourceMgr->TakeDamage(DamageAmount);
		
		UE_LOG(LogTemp, Warning, TEXT("Player took %.1f damage, WP reduced"), DamageAmount);
		
		// Return the actual damage dealt
		return DamageAmount;
	}
	
	return 0.0f;
}

void ABlackholePlayerCharacter::ApplyStagger(float Duration)
{
	// Use StatusEffectComponent instead
	if (StatusEffectComponent)
	{
		StatusEffectComponent->ApplyStatusEffect(EStatusEffectType::Stagger, Duration);
	}
}

bool ABlackholePlayerCharacter::IsStaggered() const
{
	if (StatusEffectComponent)
	{
		return StatusEffectComponent->IsStaggered();
	}
	return false;
}

void ABlackholePlayerCharacter::HandleWKeyPress()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	// Check if this is a double tap
	if (CurrentTime - LastWKeyPressTime <= DoubleTapWindow)
	{
		// Double tap detected - toggle run mode
		bIsRunning = !bIsRunning;
		SetMovementSpeed(bIsRunning);
		
		UE_LOG(LogTemp, Warning, TEXT("Double tap detected! Running: %s"), bIsRunning ? TEXT("ON") : TEXT("OFF"));
	}
	
	LastWKeyPressTime = CurrentTime;
}

void ABlackholePlayerCharacter::SetMovementSpeed(bool bRun)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		float NewSpeed = bRun ? RunSpeed : WalkSpeed;
		Movement->MaxWalkSpeed = NewSpeed;
		MaxWalkSpeed = NewSpeed; // Update our stored value
		
		UE_LOG(LogTemp, Warning, TEXT("Movement speed set to: %.0f (Running: %s)"), 
			NewSpeed, bRun ? TEXT("YES") : TEXT("NO"));
	}
}

