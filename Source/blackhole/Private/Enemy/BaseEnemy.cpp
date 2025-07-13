#include "Enemy/BaseEnemy.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Systems/ThresholdManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Config/GameplayConfig.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Enemy/AI/BlackholeAIController.h"
#include "Data/EnemyStatsData.h"
#include "Engine/DataTable.h"

ABaseEnemy::ABaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	IntegrityComponent = CreateDefaultSubobject<UIntegrityComponent>(TEXT("Integrity"));
	
	// Don't create state machine here - derived classes create their own specific types
	
	// Tag all enemies so hack abilities can identify them
	Tags.Add(FName("Enemy"));
	
	bIsDead = false;
	
	// Create sword mesh component - all enemies have swords
	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sword"));
	SwordMesh->SetupAttachment(GetMesh(), FName("weaponsocket"));
	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordMesh->SetCastShadow(true);
	
	// Set default AI update rate from config
	AIUpdateRate = GameplayConfig::Enemy::AI_UPDATE_RATE;
	
	// Set AI controller class
	AIControllerClass = ABlackholeAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	// Default minimum engagement distance
	MinimumEngagementDistance = 100.0f;
	
	// Data table configuration
	EnemyStatsDataTable = nullptr;
	StatsRowName = NAME_None;
	
	// Configure movement component for smooth enemy movement
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// Enable rotation towards movement direction
		Movement->bOrientRotationToMovement = true;
		
		// Set smooth rotation rate to prevent twitching
		Movement->RotationRate = FRotator(0.0f, 270.0f, 0.0f); // Smooth 270 deg/sec turn rate
		
		// Disable controller rotation to prevent conflicts
		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = false;
		bUseControllerRotationRoll = false;
		
		// Set acceleration for smooth starts/stops
		Movement->MaxAcceleration = 800.0f;
		Movement->BrakingDecelerationWalking = 800.0f;
		
		// Enable path following for smoother navigation
		Movement->bUseRVOAvoidance = true;
		Movement->AvoidanceWeight = 0.5f;
	}
}

void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	// Load stats from data table if configured
	if (EnemyStatsDataTable && !StatsRowName.IsNone())
	{
		LoadStatsFromDataTable();
	}
	
	// Store default walk speed
	if (GetCharacterMovement())
	{
		DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
		UE_LOG(LogTemp, Warning, TEXT("%s: Default walk speed set to %.0f"), *GetName(), DefaultWalkSpeed);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: No CharacterMovement component found!"), *GetName());
	}
	
	// Verify AI Controller
	if (GetController())
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: AIController present: %s"), *GetName(), *AIController->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s: Controller is not an AIController!"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: No controller found! AutoPossessAI: %d"), *GetName(), (int32)AutoPossessAI);
	}
	
	// Try to find player if no target set
	if (!TargetActor)
	{
		if (UWorld* World = GetWorld())
		{
			TargetActor = UGameplayStatics::GetPlayerCharacter(World, 0);
			if (TargetActor)
			{
				UE_LOG(LogTemp, Log, TEXT("%s: Found player target in BeginPlay"), *GetName());
			}
		}
	}
	
	// Set target for state machine
	if (StateMachine)
	{
		if (TargetActor)
		{
			StateMachine->SetTarget(TargetActor);
			UE_LOG(LogTemp, Warning, TEXT("%s: Target set for state machine in BeginPlay - %s"), *GetName(), *TargetActor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s: No target actor to set for state machine!"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: No state machine component found after CreateStateMachine()!"), *GetName());
	}
	
	// Bind to integrity component's OnReachedZero event
	if (IntegrityComponent)
	{
		IntegrityComponent->OnReachedZero.AddDynamic(this, &ABaseEnemy::OnDeath);
	}
	
	// State machine handles AI updates now - no need for timer
	// Legacy AI timer disabled in favor of state machine
}

void ABaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// AI updates are now handled by timer
	// This tick function is only kept for movement/animation purposes
}

void ABaseEnemy::UpdateAIBehavior(float DeltaTime)
{
	// Legacy method - kept for compatibility
	// State machine now handles all AI behavior
	// This method is only called if derived classes haven't migrated yet
	
	// Check if we can see the player and start combat if needed
	if (TargetActor && !bHasStartedCombat && StateMachine)
	{
		if (StateMachine->HasLineOfSight())
		{
			float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
			if (Distance < GameplayConfig::Enemy::DETECTION_RANGE)
			{
				// Start combat when first enemy sees player
				if (UWorld* World = GetWorld())
				{
					if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
					{
						if (!ThresholdMgr->IsInCombat())
						{
							ThresholdMgr->StartCombat();
							UE_LOG(LogTemp, Warning, TEXT("Combat Started - %s detected player!"), *GetName());
							
							// Show on screen message
							if (GEngine)
							{
								// Debug message removed - combat started
							}
						}
					}
				}
				bHasStartedCombat = true;
				OnCombatStart();
			}
		}
	}
}

AActor* ABaseEnemy::GetTargetActor() const
{
	return TargetActor;
}

void ABaseEnemy::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void ABaseEnemy::OnDeath()
{
	if (bIsDead)
	{
		return;
	}
	
	bIsDead = true;
	
	// Notify state machine
	if (StateMachine)
	{
		StateMachine->ChangeState(EEnemyState::Dead);
	}
	
	// Stop AI updates
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AIUpdateTimer);
	}
	
	// Stop all movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	
	// Disable collision on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Enable ragdoll physics
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	
	// Add some impulse to make the death more dramatic
	FVector ImpulseDirection = GetActorLocation() - (TargetActor ? TargetActor->GetActorLocation() : GetActorLocation());
	ImpulseDirection.Normalize();
	ImpulseDirection.Z = GameplayConfig::Enemy::DEATH_IMPULSE_Z; // Add some upward force
	GetMesh()->AddImpulse(ImpulseDirection * GameplayConfig::Enemy::DEATH_IMPULSE_MAGNITUDE, NAME_None, true);
	
	// Destroy actor after delay
	SetLifeSpan(GameplayConfig::Enemy::CORPSE_LIFESPAN);
}

void ABaseEnemy::TimerUpdateAI()
{
	if (!bIsDead)
	{
		// Call the existing UpdateAIBehavior with a fixed delta time
		UpdateAIBehavior(AIUpdateRate);
	}
}

ABlackholePlayerCharacter* ABaseEnemy::GetTargetPlayer() const
{
	return Cast<ABlackholePlayerCharacter>(TargetActor);
}

void ABaseEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AIUpdateTimer);
		GetWorld()->GetTimerManager().ClearTimer(SpeedResetTimerHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

float ABaseEnemy::GetDefaultWalkSpeed() const
{
	return DefaultWalkSpeed;
}

void ABaseEnemy::ApplyMovementSpeedModifier(float Multiplier, float Duration)
{
	if (!GetCharacterMovement() || !GetWorld()) return;
	
	// Clear any existing speed reset timer
	GetWorld()->GetTimerManager().ClearTimer(SpeedResetTimerHandle);
	
	// Apply speed modifier
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed * Multiplier;
	
	// Reset after duration
	if (Duration > 0.0f)
	{
		// Use weak pointer to prevent crashes if enemy is destroyed
		TWeakObjectPtr<ABaseEnemy> WeakThis = this;
		float ResetSpeed = DefaultWalkSpeed;
		
		GetWorld()->GetTimerManager().SetTimer(SpeedResetTimerHandle, [WeakThis, ResetSpeed]()
		{
			if (WeakThis.IsValid() && WeakThis->GetCharacterMovement())
			{
				WeakThis->GetCharacterMovement()->MaxWalkSpeed = ResetSpeed;
			}
		}, Duration, false);
	}
}

void ABaseEnemy::LoadStatsFromDataTable()
{
	if (!EnemyStatsDataTable || StatsRowName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: No data table or row name configured"), *GetName());
		return;
	}
	
	FEnemyStatsData Stats = UEnemyStatsManager::GetEnemyStats(this, EnemyStatsDataTable, StatsRowName);
	UEnemyStatsManager::ApplyStatsToEnemy(this, Stats);
	
	UE_LOG(LogTemp, Log, TEXT("%s: Loaded stats from data table row '%s'"), *GetName(), *StatsRowName.ToString());
}