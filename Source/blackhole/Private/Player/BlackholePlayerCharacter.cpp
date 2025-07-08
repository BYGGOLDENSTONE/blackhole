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
#include "Components/Abilities/AbilityComponent.h"

ABlackholePlayerCharacter::ABlackholePlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.0f; // Base jump velocity for movement component
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
	WeaponSocketName = "weaponsocket"; // Default socket name - adjust in Blueprint if different
	
	// Create maze weapon mesh component (Forge path)
	MazeWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MazeWeapon"));
	MazeWeaponMesh->SetupAttachment(GetMesh());
	MazeWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MazeWeaponMesh->SetCastShadow(true);
	
	// Create katana weapon mesh component (Hacker path)
	KatanaWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaWeapon"));
	KatanaWeaponMesh->SetupAttachment(GetMesh());
	KatanaWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	KatanaWeaponMesh->SetCastShadow(true);
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
	
	// Attach weapons to the specified socket
	if (GetMesh() && !WeaponSocketName.IsNone())
	{
		if (MazeWeaponMesh)
		{
			MazeWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
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
}

void ABlackholePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Check if player should die
	if (!bIsDead)
	{
		CheckIntegrity();
	}
	
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
	
	// Update weapon visibility based on camera mode
	UpdateWeaponVisibility();
}

void ABlackholePlayerCharacter::SetHeadVisibility(bool bVisible)
{
	if (GetMesh() && !HeadBoneName.IsNone())
	{
		// Hide/show the head bone and all its children
		GetMesh()->HideBoneByName(HeadBoneName, bVisible ? EPhysBodyOp::PBO_None : EPhysBodyOp::PBO_Term);
	}
}

void ABlackholePlayerCharacter::ExecutePathBasedAbility(UAbilityComponent* HackerAbility, UAbilityComponent* ForgeAbility)
{
	// Unified path checker for ability execution
	UAbilityComponent* AbilityToExecute = nullptr;
	
	if (CurrentPath == ECharacterPath::Hacker && HackerAbility)
	{
		AbilityToExecute = HackerAbility;
	}
	else if (CurrentPath == ECharacterPath::Forge && ForgeAbility)
	{
		AbilityToExecute = ForgeAbility;
	}
	
	// Execute the ability and register it with combo tracker
	if (AbilityToExecute)
	{
		AbilityToExecute->Execute();
		if (ComboTracker)
		{
			ComboTracker->RegisterAbilityUse(AbilityToExecute);
		}
	}
}

void ABlackholePlayerCharacter::UseDash()
{
	// Use unified path checker for dash abilities
	ExecutePathBasedAbility(HackerDashAbility, ForgeDashAbility);
}

void ABlackholePlayerCharacter::UseUtilityJump()
{
	// Use unified path checker for jump abilities
	ExecutePathBasedAbility(HackerJumpAbility, ForgeJumpAbility);
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
	// Left Mouse Button - Basic attack
	// Currently: Slash (shared) | Future: Katana Slash (Hacker) / Forge Slam (Forge)
	// For now, use the shared Slash ability for both paths
	ExecutePathBasedAbility(SlashAbility, SlashAbility);
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
	// Hacker: Reserved | Forge: Hammer Strike
	ExecutePathBasedAbility(nullptr, HammerStrikeAbility);
}

void ABlackholePlayerCharacter::UseAbilitySlot6()
{
	// F key - Reserved for ultimate abilities
	// Hacker: Reserved | Forge: Reserved
	// TODO: Implement ultimate abilities
	ExecutePathBasedAbility(nullptr, nullptr);
}

void ABlackholePlayerCharacter::UpdateWeaponVisibility()
{
	// Show/hide weapons based on current path
	if (MazeWeaponMesh && KatanaWeaponMesh)
	{
		if (CurrentPath == ECharacterPath::Hacker)
		{
			// Hacker path uses Katana
			KatanaWeaponMesh->SetVisibility(true);
			MazeWeaponMesh->SetVisibility(false);
		}
		else // Forge path
		{
			// Forge path uses Maze weapon
			MazeWeaponMesh->SetVisibility(true);
			KatanaWeaponMesh->SetVisibility(false);
		}
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
	
	// Disable all abilities
	TArray<UActorComponent*> AllComponents = GetComponents().Array();
	TArray<UActorComponent*> AbilityComponents = AllComponents.FilterByPredicate([](UActorComponent* Component)
	{
		return Component && Component->IsA<UAbilityComponent>();
	});
	
	for (UActorComponent* Component : AbilityComponents)
	{
		if (UAbilityComponent* Ability = Cast<UAbilityComponent>(Component))
		{
			Ability->SetComponentTickEnabled(false);
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
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	// Show death message
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("YOU DIED!"));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Press ESC to quit"));
	}
}

void ABlackholePlayerCharacter::CheckIntegrity()
{
	if (IntegrityComponent && IntegrityComponent->GetCurrentValue() <= 0.0f)
	{
		Die();
	}
}

void ABlackholePlayerCharacter::OnThresholdDeath()
{
	UE_LOG(LogTemp, Error, TEXT("Player death triggered by ThresholdManager!"));
	Die();
}