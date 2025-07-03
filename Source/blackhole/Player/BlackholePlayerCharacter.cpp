#include "BlackholePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/StaticMeshComponent.h"
#include "../Components/Attributes/IntegrityComponent.h"
#include "../Components/Attributes/StaminaComponent.h"
#include "../Components/Attributes/WillPowerComponent.h"
#include "../Components/Abilities/SlashAbilityComponent.h"
#include "../Components/Abilities/SystemFreezeAbilityComponent.h"
#include "../Components/Abilities/KillAbilityComponent.h"

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

	SlashAbility = CreateDefaultSubobject<USlashAbilityComponent>(TEXT("SlashAbility"));
	SystemFreezeAbility = CreateDefaultSubobject<USystemFreezeAbilityComponent>(TEXT("SystemFreezeAbility"));
	KillAbility = CreateDefaultSubobject<UKillAbilityComponent>(TEXT("KillAbility"));

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
}

void ABlackholePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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

		// Abilities
		EnhancedInputComponent->BindAction(SlashAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseSlash);
		EnhancedInputComponent->BindAction(SystemFreezeAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseSystemFreeze);
		EnhancedInputComponent->BindAction(KillAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::UseKill);

		// Camera toggle
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ABlackholePlayerCharacter::ToggleCamera);
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

void ABlackholePlayerCharacter::UseSlash()
{
	if (SlashAbility)
	{
		SlashAbility->Execute();
	}
}

void ABlackholePlayerCharacter::UseSystemFreeze()
{
	if (SystemFreezeAbility)
	{
		SystemFreezeAbility->Execute();
	}
}

void ABlackholePlayerCharacter::UseKill()
{
	if (KillAbility)
	{
		KillAbility->Execute();
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