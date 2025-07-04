#include "Player/BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/StaticMeshComponent.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Attributes/StaminaComponent.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Components/Attributes/HeatComponent.h"
#include "Systems/ResourceManager.h"
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
#include "Components/Abilities/Player/Forge/MoltenMaceSlashAbility.h"
#include "Components/Abilities/Player/Forge/HeatShieldAbility.h"
#include "Components/Abilities/Player/Forge/BlastChargeAbility.h"
#include "Components/Abilities/Player/Forge/HammerStrikeAbility.h"
#include "Systems/ComboTracker.h"

ABlackholePlayerCharacter::ABlackholePlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 400.0f;
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
	MoltenMaceSlashAbility = CreateDefaultSubobject<UMoltenMaceSlashAbility>(TEXT("MoltenMaceSlashAbility"));
	HeatShieldAbility = CreateDefaultSubobject<UHeatShieldAbility>(TEXT("HeatShieldAbility"));
	BlastChargeAbility = CreateDefaultSubobject<UBlastChargeAbility>(TEXT("BlastChargeAbility"));
	HammerStrikeAbility = CreateDefaultSubobject<UHammerStrikeAbility>(TEXT("HammerStrikeAbility"));

	// Default to Hacker path
	CurrentPath = ECharacterPath::Hacker;
	
	// Create combo tracker
	ComboTracker = CreateDefaultSubobject<UComboTracker>(TEXT("ComboTracker"));

	bIsFirstPerson = false;
	FirstPersonCameraOffset = 70.0f;
	HeadBoneName = "head"; // Default head bone name - adjust in Blueprint if different
	
	// Create maze weapon mesh component
	MazeWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MazeWeapon"));
	MazeWeaponMesh->SetupAttachment(GetMesh(), FName("weaponsocket"));
	MazeWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MazeWeaponMesh->SetCastShadow(true);
}

void ABlackholePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

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
}

void ABlackholePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
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
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

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
	bIsFirstPerson = !bIsFirstPerson;
	
	if (bIsFirstPerson)
	{
		// Switch to first person
		// Detach camera from spring arm and attach directly to mesh
		CameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		
		// Attach camera to head socket if it exists, otherwise use relative position
		FName HeadSocket = "camerasocket"; // Your custom socket name
		if (GetMesh()->DoesSocketExist(HeadSocket))
		{
			CameraComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HeadSocket);
			// Position camera slightly forward and up from head socket to avoid clipping
			CameraComponent->SetRelativeLocation(FVector(15.0f, 0.0f, 5.0f));
			CameraComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
		else
		{
			// Fallback: attach to mesh at approximate head height
			CameraComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
			// Adjust these values based on your character model
			CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f)); // Approximate head height
			CameraComponent->SetRelativeRotation(FRotator(-10.0f, 90.0f, 0.0f)); // Look slightly down, face forward
		}
		
		// Make camera use controller rotation directly
		CameraComponent->bUsePawnControlRotation = true;
		
		// Disable spring arm influence
		SpringArmComponent->bUsePawnControlRotation = false;
		SpringArmComponent->bInheritPitch = false;
		SpringArmComponent->bInheritYaw = false;
		SpringArmComponent->bInheritRoll = false;
		
		// Don't hide character mesh in first person - we want to see arms/legs
		GetMesh()->SetOwnerNoSee(false);
		
		// Hide the head bone to avoid seeing inside it
		SetHeadVisibility(false);
		
		// Adjust movement to feel better in first person
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		// Switch to third person
		// Reattach camera to spring arm
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
		SpringArmComponent->TargetArmLength = 400.0f;
		SpringArmComponent->SocketOffset = FVector::ZeroVector;
		
		// Ensure character mesh is visible in third person
		GetMesh()->SetOwnerNoSee(false);
		
		// Show the head bone again
		SetHeadVisibility(true);
		
		// Reset movement settings
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void ABlackholePlayerCharacter::SetHeadVisibility(bool bVisible)
{
	if (GetMesh() && !HeadBoneName.IsNone())
	{
		// Hide/show the head bone and all its children
		GetMesh()->HideBoneByName(HeadBoneName, bVisible ? EPhysBodyOp::PBO_None : EPhysBodyOp::PBO_Term);
	}
}

void ABlackholePlayerCharacter::UseDash()
{
	// Use the appropriate dash based on current path
	if (CurrentPath == ECharacterPath::Hacker && HackerDashAbility)
	{
		HackerDashAbility->Execute();
	}
	else if (CurrentPath == ECharacterPath::Forge && ForgeDashAbility)
	{
		ForgeDashAbility->Execute();
	}
}

void ABlackholePlayerCharacter::UseUtilityJump()
{
	// Use the appropriate jump based on current path
	if (CurrentPath == ECharacterPath::Hacker && HackerJumpAbility)
	{
		HackerJumpAbility->Execute();
	}
	else if (CurrentPath == ECharacterPath::Forge && ForgeJumpAbility)
	{
		ForgeJumpAbility->Execute();
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
	
	// Optional: Broadcast an event for UI updates
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
			FString::Printf(TEXT("Path Changed: %s"), *GetCurrentPathName()));
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
	// Left Mouse Button - Basic attack (Slash)
	if (SlashAbility)
	{
		SlashAbility->Execute();
		if (ComboTracker)
		{
			ComboTracker->RegisterAbilityUse(SlashAbility);
		}
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot2()
{
	// Right Mouse Button - Secondary attack
	if (CurrentPath == ECharacterPath::Hacker)
	{
		// Firewall Breach for Hacker path
		if (FirewallBreachAbility)
		{
			FirewallBreachAbility->Execute();
			if (ComboTracker)
			{
				ComboTracker->RegisterAbilityUse(FirewallBreachAbility);
			}
		}
	}
	else if (CurrentPath == ECharacterPath::Forge)
	{
		// Molten Mace Slash for Forge path
		if (MoltenMaceSlashAbility)
		{
			MoltenMaceSlashAbility->Execute();
			if (ComboTracker)
			{
				ComboTracker->RegisterAbilityUse(MoltenMaceSlashAbility);
			}
		}
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot3()
{
	// Q key
	if (CurrentPath == ECharacterPath::Hacker)
	{
		// Pulse Hack for Hacker path
		if (PulseHackAbility)
		{
			PulseHackAbility->Execute();
			if (ComboTracker)
			{
				ComboTracker->RegisterAbilityUse(PulseHackAbility);
			}
		}
	}
	else if (CurrentPath == ECharacterPath::Forge)
	{
		// Heat Shield for Forge path
		if (HeatShieldAbility)
		{
			HeatShieldAbility->Execute();
			if (ComboTracker)
			{
				ComboTracker->RegisterAbilityUse(HeatShieldAbility);
			}
		}
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot4()
{
	// E key
	if (CurrentPath == ECharacterPath::Hacker)
	{
		// Gravity Pull for Hacker path
		if (GravityPullAbility)
		{
			GravityPullAbility->Execute();
			if (ComboTracker)
			{
				ComboTracker->RegisterAbilityUse(GravityPullAbility);
			}
		}
	}
	else if (CurrentPath == ECharacterPath::Forge)
	{
		// Blast Charge for Forge path
		if (BlastChargeAbility)
		{
			BlastChargeAbility->Execute();
			if (ComboTracker)
			{
				ComboTracker->RegisterAbilityUse(BlastChargeAbility);
			}
		}
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot5()
{
	// R key
	if (CurrentPath == ECharacterPath::Hacker)
	{
		// Reserved for future Hacker ability
		// TODO: Add another hacker ability
	}
	else if (CurrentPath == ECharacterPath::Forge)
	{
		// Hammer Strike for Forge path
		if (HammerStrikeAbility)
		{
			HammerStrikeAbility->Execute();
			if (ComboTracker)
			{
				ComboTracker->RegisterAbilityUse(HammerStrikeAbility);
			}
		}
	}
}

void ABlackholePlayerCharacter::UseAbilitySlot6()
{
	// F key - Reserved for both paths
	// This could be for ultimate abilities or special moves
	// TODO: Implement ultimate abilities
}