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
#include "Components/StaticMeshComponent.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Attributes/StaminaComponent.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Components/Attributes/HeatComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Systems/GameStateManager.h"
#include "UI/BlackholeHUD.h"
#include "Components/Abilities/Player/Basic/SlashAbilityComponent.h"
// #include "Components/Abilities/Player/SystemFreezeAbilityComponent.h" // Removed
#include "Components/Abilities/Player/Basic/KillAbilityComponent.h"
#include "Components/Abilities/Player/Hacker/GravityPullAbilityComponent.h"
#include "Components/Abilities/Player/Utility/HackerDashAbility.h"
#include "Components/Abilities/Player/Utility/ForgeDashAbility.h"
#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "Components/Abilities/Player/Utility/ForgeJumpAbility.h"
#include "Components/Abilities/Player/Hacker/PulseHackAbility.h"
#include "Components/Abilities/Player/Hacker/FirewallBreachAbility.h"
#include "Components/Abilities/Player/Hacker/DataSpikeAbility.h"
#include "Components/Abilities/Player/Hacker/SystemOverrideAbility.h"
#include "Components/Abilities/Player/Forge/MoltenMaceSlashAbility.h"
#include "Components/Abilities/Player/Forge/HeatShieldAbility.h"
#include "Components/Abilities/Player/Forge/BlastChargeAbility.h"
#include "Components/Abilities/Player/Forge/HammerStrikeAbility.h"
#include "Systems/ComboTracker.h"
#include "Systems/ComboSystem.h"
#include "Components/ComboComponent.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Config/GameplayConfig.h"

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

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = GameplayConfig::Movement::CAMERA_ARM_LENGTH;
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;

	IntegrityComponent = CreateDefaultSubobject<UIntegrityComponent>(TEXT("Integrity"));
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("Stamina"));
	WillPowerComponent = CreateDefaultSubobject<UWillPowerComponent>(TEXT("WillPower"));
	HeatComponent = CreateDefaultSubobject<UHeatComponent>(TEXT("Heat"));

	SlashAbility = CreateDefaultSubobject<USlashAbilityComponent>(TEXT("SlashAbility"));
	// SystemFreezeAbility removed
	KillAbility = CreateDefaultSubobject<UKillAbilityComponent>(TEXT("KillAbility"));
	GravityPullAbility = CreateDefaultSubobject<UGravityPullAbilityComponent>(TEXT("GravityPullAbility"));

	// Create all utility abilities
	HackerDashAbility = CreateDefaultSubobject<UHackerDashAbility>(TEXT("HackerDashAbility"));
	ForgeDashAbility = CreateDefaultSubobject<UForgeDashAbility>(TEXT("ForgeDashAbility"));
	HackerJumpAbility = CreateDefaultSubobject<UHackerJumpAbility>(TEXT("HackerJumpAbility"));
	ForgeJumpAbility = CreateDefaultSubobject<UForgeJumpAbility>(TEXT("ForgeJumpAbility"));

	// Create path-specific combat abilities
	PulseHackAbility = CreateDefaultSubobject<UPulseHackAbility>(TEXT("PulseHackAbility"));
	FirewallBreachAbility = CreateDefaultSubobject<UFirewallBreachAbility>(TEXT("FirewallBreachAbility"));
	DataSpikeAbility = CreateDefaultSubobject<UDataSpikeAbility>(TEXT("DataSpikeAbility"));
	SystemOverrideAbility = CreateDefaultSubobject<USystemOverrideAbility>(TEXT("SystemOverrideAbility"));
	MoltenMaceSlashAbility = CreateDefaultSubobject<UMoltenMaceSlashAbility>(TEXT("MoltenMaceSlashAbility"));
	HeatShieldAbility = CreateDefaultSubobject<UHeatShieldAbility>(TEXT("HeatShieldAbility"));
	BlastChargeAbility = CreateDefaultSubobject<UBlastChargeAbility>(TEXT("BlastChargeAbility"));
	HammerStrikeAbility = CreateDefaultSubobject<UHammerStrikeAbility>(TEXT("HammerStrikeAbility"));

	// Default to Hacker path
	CurrentPath = ECharacterPath::Hacker;
	
	// Create combo tracker
	ComboTracker = CreateDefaultSubobject<UComboTracker>(TEXT("ComboTracker"));
	
	// Create new combo system
	ComboSystem = CreateDefaultSubobject<UComboSystem>(TEXT("ComboSystem"));
	
	// Create component-based combo system
	ComboComponent = CreateDefaultSubobject<UComboComponent>(TEXT("ComboComponent"));

	bIsFirstPerson = false;
	FirstPersonCameraOffset = GameplayConfig::Movement::FIRST_PERSON_OFFSET;
	HeadBoneName = "head"; // Default head bone name - adjust in Blueprint if different
	WeaponSocketName = "weaponsocket"; // Default socket name - adjust in Blueprint if different
	
	// Create mace weapon mesh component (Forge path)
	MaceWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaceWeapon"));
	MaceWeaponMesh->SetupAttachment(GetMesh());
	MaceWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MaceWeaponMesh->SetCastShadow(true);
	
	// Create katana weapon mesh component (Hacker path)
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
	
	// Sync initial path with ResourceManager
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			ResourceMgr->SetCurrentPath(CurrentPath);
		}
	}
	
	// Attach weapons to the specified socket
	if (GetMesh() && !WeaponSocketName.IsNone())
	{
		if (MaceWeaponMesh)
		{
			MaceWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
		}
		if (KatanaWeaponMesh)
		{
			KatanaWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
		}
	}
	
	// Set initial weapon visibility based on current path
	UpdateWeaponVisibility();
	
	// Bind to ThresholdManager death event
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			ThresholdMgr->OnPlayerDeath.AddDynamic(this, &ABlackholePlayerCharacter::OnThresholdDeath);
		}
	}
	
	// Bind to integrity component's OnReachedZero event
	if (IntegrityComponent)
	{
		IntegrityComponent->OnReachedZero.AddDynamic(this, &ABlackholePlayerCharacter::Die);
	}
	
	// Bind to combo component events
	if (IsValid(ComboComponent))
	{
		ComboComponent->OnComboPerformed.AddDynamic(this, &ABlackholePlayerCharacter::OnComboPerformed);
	}
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
	if (IsValid(ComboComponent))
	{
		ComboComponent->OnComboPerformed.RemoveDynamic(this, &ABlackholePlayerCharacter::OnComboPerformed);
	}
	
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			ThresholdMgr->OnPlayerDeath.RemoveDynamic(this, &ABlackholePlayerCharacter::OnThresholdDeath);
		}
	}
	
	if (IsValid(IntegrityComponent))
	{
		IntegrityComponent->OnReachedZero.RemoveDynamic(this, &ABlackholePlayerCharacter::Die);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ABlackholePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Death is now handled by event from IntegrityComponent
	
	// Dissipate heat through ResourceManager
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			ResourceMgr->DissipateHeat(DeltaTime);
		}
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
		
		// Utility abilities
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseDash);
		EnhancedInputComponent->BindAction(UtilityJumpAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseUtilityJump);
		
		// Path switching
		EnhancedInputComponent->BindAction(SwitchPathAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::SwitchPath);
		
		// Menu toggle (Quote key)
		EnhancedInputComponent->BindAction(MenuToggleAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::ToggleMenu);
		
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
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

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
	if (KillAbility)
	{
		KillAbility->Execute();
		if (ComboTracker)
		{
			ComboTracker->RegisterAbilityUse(KillAbility);
		}
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
		}
		
		// Re-enable spring arm
		if (IsValid(SpringArmComponent))
		{
			SpringArmComponent->bUsePawnControlRotation = true;
			SpringArmComponent->bInheritPitch = true;
			SpringArmComponent->bInheritYaw = true;
			SpringArmComponent->bInheritRoll = true;
			SpringArmComponent->TargetArmLength = GameplayConfig::Movement::CAMERA_ARM_LENGTH;
			SpringArmComponent->SocketOffset = FVector::ZeroVector;
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
	
	// Update weapon visibility based on camera mode
	UpdateWeaponVisibility();
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

void ABlackholePlayerCharacter::ExecutePathBasedAbility(UAbilityComponent* HackerAbility, UAbilityComponent* ForgeAbility)
{
	// Unified path checker for ability execution
	UAbilityComponent* AbilityToExecute = nullptr;
	
	if (CurrentPath == ECharacterPath::Hacker && IsValid(HackerAbility))
	{
		AbilityToExecute = HackerAbility;
	}
	else if (CurrentPath == ECharacterPath::Forge && IsValid(ForgeAbility))
	{
		AbilityToExecute = ForgeAbility;
	}
	
	// Execute the ability and register it with combo tracker
	if (IsValid(AbilityToExecute))
	{
		AbilityToExecute->Execute();
		if (IsValid(ComboTracker))
		{
			ComboTracker->RegisterAbilityUse(AbilityToExecute);
		}
	}
}

void ABlackholePlayerCharacter::UseDash()
{
	// Safety check
	if (!IsValid(this) || bIsDead)
	{
		return;
	}
	
	// Get the ability we're about to execute
	UAbilityComponent* AbilityToExecute = nullptr;
	if (CurrentPath == ECharacterPath::Hacker && IsValid(HackerDashAbility))
	{
		AbilityToExecute = HackerDashAbility;
	}
	else if (CurrentPath == ECharacterPath::Forge && IsValid(ForgeDashAbility))
	{
		AbilityToExecute = ForgeDashAbility;
	}
	
	// Only register input if ability can execute
	if (IsValid(AbilityToExecute) && AbilityToExecute->CanExecute())
	{
		// Register input with combo system BEFORE execution
		if (IsValid(ComboComponent))
		{
			ComboComponent->RegisterInput("Dash");
		}
		
		// Execute the ability
		ExecutePathBasedAbility(HackerDashAbility, ForgeDashAbility);
	}
}

void ABlackholePlayerCharacter::UseUtilityJump()
{
	// Safety check
	if (!IsValid(this) || bIsDead)
	{
		return;
	}
	
	// Get the ability we're about to execute
	UAbilityComponent* AbilityToExecute = nullptr;
	if (CurrentPath == ECharacterPath::Hacker && IsValid(HackerJumpAbility))
	{
		AbilityToExecute = HackerJumpAbility;
	}
	else if (CurrentPath == ECharacterPath::Forge && IsValid(ForgeJumpAbility))
	{
		AbilityToExecute = ForgeJumpAbility;
	}
	
	// Only register input if ability can execute
	if (IsValid(AbilityToExecute) && AbilityToExecute->CanExecute())
	{
		// Register input with combo system BEFORE execution
		if (IsValid(ComboComponent))
		{
			ComboComponent->RegisterInput("Jump");
		}
		
		// Execute the ability
		ExecutePathBasedAbility(HackerJumpAbility, ForgeJumpAbility);
	}
}

void ABlackholePlayerCharacter::SwitchPath()
{
	// Toggle between Hacker and Forge paths
	if (CurrentPath == ECharacterPath::Hacker)
	{
		CurrentPath = ECharacterPath::Forge;
		UE_LOG(LogTemp, Log, TEXT("Switched to Forge path"));
	}
	else
	{
		CurrentPath = ECharacterPath::Hacker;
		UE_LOG(LogTemp, Log, TEXT("Switched to Hacker path"));
	}
	
	// Update ResourceManager's path
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			ResourceMgr->SetCurrentPath(CurrentPath);
			
			// Reset heat when switching to Hacker path
			if (CurrentPath == ECharacterPath::Hacker)
			{
				ResourceMgr->AddHeat(-ResourceMgr->GetCurrentHeat());
			}
		}
	}
	
	// Update weapon visibility
	UpdateWeaponVisibility();
	
	// Optional: Broadcast an event for UI updates
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
			FString::Printf(TEXT("Path Changed: %s"), *GetCurrentPathName()));
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

void ABlackholePlayerCharacter::SetCurrentPath(ECharacterPath NewPath)
{
	CurrentPath = NewPath;
	
	// Sync with ResourceManager
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			ResourceMgr->SetCurrentPath(CurrentPath);
			
			// Reset heat when setting to Hacker path
			if (CurrentPath == ECharacterPath::Hacker)
			{
				ResourceMgr->AddHeat(-ResourceMgr->GetCurrentHeat());
			}
		}
	}
	
	// Update weapon visibility when path is set
	UpdateWeaponVisibility();
}

FString ABlackholePlayerCharacter::GetCurrentPathName() const
{
	switch (CurrentPath)
	{
		case ECharacterPath::Hacker:
			return TEXT("Hacker");
		case ECharacterPath::Forge:
			return TEXT("Forge");
		default:
			return TEXT("None");
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
		// Register input with combo system BEFORE execution
		if (IsValid(ComboComponent))
		{
			ComboComponent->RegisterInput("Slash");
		}
		
		// Execute the ability
		ExecutePathBasedAbility(SlashAbility, SlashAbility);
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot2()
{
	// Right Mouse Button - Secondary attack
	// Hacker: Firewall Breach | Forge: Molten Mace Slash
	ExecutePathBasedAbility(FirewallBreachAbility, MoltenMaceSlashAbility);
}

void ABlackholePlayerCharacter::UseAbilitySlot3()
{
	// Q key
	// Hacker: Pulse Hack | Forge: Heat Shield
	ExecutePathBasedAbility(PulseHackAbility, HeatShieldAbility);
}

void ABlackholePlayerCharacter::UseAbilitySlot4()
{
	// E key
	// Hacker: Gravity Pull | Forge: Blast Charge
	ExecutePathBasedAbility(GravityPullAbility, BlastChargeAbility);
}

void ABlackholePlayerCharacter::UseAbilitySlot5()
{
	// R key
	// Hacker: Data Spike | Forge: Hammer Strike
	ExecutePathBasedAbility(DataSpikeAbility, HammerStrikeAbility);
}

void ABlackholePlayerCharacter::UseAbilitySlot6()
{
	// F key - Ultimate abilities
	// Hacker: System Override | Forge: Reserved
	// TODO: Implement Forge F key ability
	ExecutePathBasedAbility(SystemOverrideAbility, nullptr);
}

void ABlackholePlayerCharacter::UpdateWeaponVisibility()
{
	// Safety checks - don't update weapon visibility if character is invalid or dead
	if (!IsValid(this) || bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot update weapon visibility - character is invalid or dead"));
		return;
	}
	
	// Show/hide weapons based on current path
	if (IsValid(MaceWeaponMesh) && IsValid(KatanaWeaponMesh))
	{
		if (CurrentPath == ECharacterPath::Hacker)
		{
			// Hacker path uses Katana
			KatanaWeaponMesh->SetVisibility(true);
			MaceWeaponMesh->SetVisibility(false);
		}
		else // Forge path
		{
			// Forge path uses Mace weapon
			MaceWeaponMesh->SetVisibility(true);
			KatanaWeaponMesh->SetVisibility(false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot update weapon visibility - weapon mesh components are invalid"));
	}
}

void ABlackholePlayerCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}
	
	bIsDead = true;
	
	UE_LOG(LogTemp, Error, TEXT("PLAYER DEATH!"));
	
	// First, clear all timers for this actor to prevent crashes
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	
	// Unbind from all delegates BEFORE disabling components
	if (IsValid(ComboComponent))
	{
		ComboComponent->OnComboPerformed.RemoveDynamic(this, &ABlackholePlayerCharacter::OnComboPerformed);
	}
	
	if (IsValid(IntegrityComponent))
	{
		IntegrityComponent->OnReachedZero.RemoveDynamic(this, &ABlackholePlayerCharacter::Die);
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
				// Clear any timers in the ability
				if (UWorld* World = GetWorld())
				{
					World->GetTimerManager().ClearAllTimersForObject(Ability);
				}
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
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("YOU DIED!"));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Press ESC to restart"));
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

void ABlackholePlayerCharacter::OnComboPerformed(const FComboSequence& Combo)
{
	// Safety check - don't execute combos if dead or invalid
	if (!IsValid(this) || bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot execute combo - character is dead or invalid"));
		return;
	}
	
	// Handle combo execution based on combo name
	if (!IsValid(SlashAbility))
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerCharacter: SlashAbility is null or invalid!"));
		return;
	}
	
	// Check if SlashAbility component is enabled and can execute
	if (!SlashAbility->IsComponentTickEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: SlashAbility is disabled - cannot execute combo"));
		return;
	}
	
	// Additional safety check - ensure ability can execute
	if (!SlashAbility->CanExecute())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: SlashAbility cannot execute - combo cancelled"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Combo performed - %s"), *Combo.ComboName.ToString());
	
	// Execute the appropriate combo effect with additional safety checks
	if (Combo.ComboName == "PhantomStrike")
	{
		// Check if we're in a valid state for PhantomStrike (not currently in another movement ability)
		if (IsValid(GetCharacterMovement()) && GetCharacterMovement()->MovementMode == MOVE_Walking)
		{
			SlashAbility->ExecutePhantomStrike();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: Cannot execute PhantomStrike - invalid movement state"));
		}
	}
	else if (Combo.ComboName == "AerialRave")
	{
		SlashAbility->ExecuteAerialRave();
	}
	else if (Combo.ComboName == "TempestBlade")
	{
		SlashAbility->ExecuteTempestBlade();
	}
	else if (Combo.ComboName == "BladeDance")
	{
		// For blade dance, track hit count
		static int32 BladeDanceHitCount = 0;
		BladeDanceHitCount++;
		if (BladeDanceHitCount > 5) BladeDanceHitCount = 1;
		
		SlashAbility->ExecuteBladeDance(BladeDanceHitCount);
	}
}