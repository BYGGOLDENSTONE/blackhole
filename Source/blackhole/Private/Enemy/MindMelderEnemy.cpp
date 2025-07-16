#include "Enemy/MindMelderEnemy.h"
#include "Components/Abilities/Enemy/PowerfulMindmeldComponent.h"
#include "Components/Abilities/Enemy/DodgeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Enemy/EnemyUtility.h"
#include "Enemy/AI/MindMelderStateMachine.h"
#include "TimerManager.h"
#include "AIController.h"

AMindMelderEnemy::AMindMelderEnemy()
{
	// Replace base state machine with mind melder-specific one
	StateMachine = CreateDefaultSubobject<UMindMelderStateMachine>(TEXT("MindMelderStateMachine"));
	
	// Only create the abilities this enemy type should have
	PowerfulMindmeld = CreateDefaultSubobject<UPowerfulMindmeldComponent>(TEXT("PowerfulMindmeld"));
	DodgeAbility = CreateDefaultSubobject<UDodgeComponent>(TEXT("DodgeAbility"));
	
	MindmeldRange = 3000.0f;
	SafeDistance = 2000.0f;
	bIsChanneling = false;
	
	// Configure powerful mindmeld
	if (PowerfulMindmeld)
	{
		PowerfulMindmeld->CastTime = 30.0f;
		PowerfulMindmeld->ChannelRange = MindmeldRange;
		PowerfulMindmeld->bRequiresLineOfSight = true;
		PowerfulMindmeld->InterruptRange = 300.0f;
		
		// Bind events
		PowerfulMindmeld->OnMindmeldStarted.AddDynamic(this, &AMindMelderEnemy::OnMindmeldStarted);
		PowerfulMindmeld->OnMindmeldComplete.AddDynamic(this, &AMindMelderEnemy::OnMindmeldComplete);
		PowerfulMindmeld->OnMindmeldInterrupted.AddDynamic(this, &AMindMelderEnemy::OnMindmeldInterrupted);
	}

	// Configure movement settings - slow but steady
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 300.0f; // Slow movement
		Movement->MaxAcceleration = 600.0f;
		Movement->BrakingDecelerationWalking = 600.0f;
		Movement->RotationRate = FRotator(0.0f, 180.0f, 0.0f);
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
	}
	
	// Set default data table row name
	StatsRowName = FName("MindMelder");
	
	// Mind melders give high WP on kill due to extreme danger
	WPRewardOnKill = 30.0f;
	
	// Fragile but dangerous
	MaxWP = 75.0f;
}

void AMindMelderEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}

	CurrentWP = MaxWP;
}

void AMindMelderEnemy::UpdateAIBehavior(float DeltaTime)
{
	// State machine handles all AI behavior now
	Super::UpdateAIBehavior(DeltaTime);
}

float AMindMelderEnemy::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
	class AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	// Interrupt mindmeld if channeling and damaged
	if (bIsChanneling && PowerfulMindmeld)
	{
		PowerfulMindmeld->InterruptChannel();
	}
	
	// Retreat when damaged
	if (bRetreatWhenDamaged && ActualDamage > 0)
	{
		// Force retreat for a few seconds
		if (StateMachine)
		{
			StateMachine->ChangeState(EEnemyState::Retreat);
		}
		
		// Set timer to stop retreating
		GetWorld()->GetTimerManager().SetTimer(RetreatTimer, this, 
			&AMindMelderEnemy::StopRetreating, 3.0f, false);
	}
	
	return ActualDamage;
}

void AMindMelderEnemy::OnDeath()
{
	// Make sure to interrupt any ongoing mindmeld
	if (PowerfulMindmeld && PowerfulMindmeld->IsChanneling())
	{
		PowerfulMindmeld->InterruptChannel();
	}
	
	Super::OnDeath();
}

void AMindMelderEnemy::OnMindmeldStarted(float CastTime)
{
	bIsChanneling = true;
	
	// Stop moving while channeling
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	
	// Change to channeling state if available
	if (StateMachine)
	{
		StateMachine->ChangeState(EEnemyState::Channeling);
	}
	
	UE_LOG(LogTemp, Error, TEXT("%s: Beginning %.0f second mindmeld channel!"), *GetName(), CastTime);
}

void AMindMelderEnemy::OnMindmeldComplete()
{
	bIsChanneling = false;
	
	// Re-enable movement
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	
	// Return to alert state
	if (StateMachine)
	{
		StateMachine->ChangeState(EEnemyState::Alert);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%s: Mindmeld complete!"), *GetName());
}

void AMindMelderEnemy::OnMindmeldInterrupted()
{
	bIsChanneling = false;
	
	// Re-enable movement
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	
	// Retreat after interruption
	if (StateMachine)
	{
		StateMachine->ChangeState(EEnemyState::Retreat);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%s: Mindmeld interrupted!"), *GetName());
}

void AMindMelderEnemy::MaintainSafeDistance(float DeltaTime)
{
	if (!TargetActor) return;
	
	// Move away from target
	FVector DirectionAway = (GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
	FVector DesiredLocation = GetActorLocation() + DirectionAway * 200.0f * DeltaTime;
	
	// Use AI movement
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->MoveToLocation(DesiredLocation, 10.0f, false);
	}
}

bool AMindMelderEnemy::HasLineOfSightToTarget() const
{
	if (!TargetActor) return false;
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	FVector Start = GetActorLocation() + FVector(0, 0, 50);
	FVector End = TargetActor->GetActorLocation() + FVector(0, 0, 50);
	
	return !GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
}

float AMindMelderEnemy::GetDistanceToTarget() const
{
	return UEnemyUtility::GetDistanceToTarget(this, TargetActor);
}

void AMindMelderEnemy::TryDodge()
{
	if (DodgeAbility && DodgeAbility->CanExecute())
	{
		if (FMath::RandRange(0.0f, 1.0f) <= DodgeChance)
		{
			DodgeAbility->Execute();
		}
	}
}

void AMindMelderEnemy::StopRetreating()
{
	// Return to alert state after retreat timer
	if (StateMachine && StateMachine->GetCurrentState() == EEnemyState::Retreat)
	{
		StateMachine->ChangeState(EEnemyState::Alert);
	}
}