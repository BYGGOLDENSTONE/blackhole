#include "Components/Abilities/Enemy/AssassinApproachComponent.h"
#include "Components/Abilities/Enemy/StabAttackComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AgileEnemy.h"
#include "Enemy/AI/EnemyStateMachine.h"
#include "Player/BlackholePlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UAssassinApproachComponent::UAssassinApproachComponent()
{
	Cooldown = 3.0f;
	Range = 600.0f; // Maximum dash range
	WPCost = 0.0f; // No cost for enemy abilities
	bIsBasicAbility = false; // This is a special ability
}

void UAssassinApproachComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Find the stab attack component
	if (AActor* Owner = GetOwner())
	{
		StabAttackComponent = Owner->FindComponentByClass<UStabAttackComponent>();
		if (!StabAttackComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("AssassinApproach: No StabAttackComponent found on %s"), *Owner->GetName());
		}
	}
	
	// Set range from component
	Range = DashRange;
}

bool UAssassinApproachComponent::CanExecute() const
{
	if (!Super::CanExecute()) return false;
	
	// Check if we have a valid target
	if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetOwner()))
	{
		if (UEnemyStateMachine* StateMachine = Enemy->GetStateMachine())
		{
			if (AActor* Target = StateMachine->GetTarget())
			{
				float Distance = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
				return Distance <= DashRange;
			}
		}
	}
	
	return false;
}

void UAssassinApproachComponent::Execute()
{
	if (!CanExecute())
	{
		return;
	}
	
	Super::Execute();
	ExecuteDashBehind();
}

void UAssassinApproachComponent::ExecuteDashBehind()
{
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetOwner());
	if (!Enemy) return;
	
	// Get target from state machine
	AActor* Target = nullptr;
	if (UEnemyStateMachine* StateMachine = Enemy->GetStateMachine())
	{
		Target = StateMachine->GetTarget();
	}
	
	if (!Target) return;
	
	// Store target for backstab
	DashTarget = Target;
	
	FVector EnemyLocation = Enemy->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	
	// Predict target movement
	FVector TargetVelocity = FVector::ZeroVector;
	if (APawn* TargetPawn = Cast<APawn>(Target))
	{
		if (UMovementComponent* MovComp = TargetPawn->GetMovementComponent())
		{
			TargetVelocity = MovComp->Velocity;
		}
	}
	
	// Calculate dash destination behind target
	FVector PredictedLocation = TargetLocation + (TargetVelocity * AttackDelayAfterDash);
	FVector DirectionToTarget = (PredictedLocation - EnemyLocation).GetSafeNormal();
	float DistanceToTarget = FVector::Dist(EnemyLocation, PredictedLocation);
	
	// Dash past the target
	float DashDistance = DistanceToTarget + DashBehindDistance;
	FVector DashDirection = DirectionToTarget;
	
	// Apply dash impulse
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->AddImpulse(DashDirection * DashForce, true);
		
		UE_LOG(LogTemp, Warning, TEXT("AssassinApproach: Dashing %.0f units behind target"), DashDistance);
	}
	
	// Schedule backstab attack
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashAttackTimerHandle);
		
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &UAssassinApproachComponent::ExecuteBackstab);
		World->GetTimerManager().SetTimer(DashAttackTimerHandle, TimerDelegate, AttackDelayAfterDash, false);
	}
	
	#if WITH_EDITOR
	// Debug visualization
	DrawDebugLine(GetWorld(), EnemyLocation, EnemyLocation + (DashDirection * DashDistance), 
		FColor::Purple, false, 2.0f, 0, 2.0f);
	#endif
}

void UAssassinApproachComponent::ExecuteBackstab()
{
	if (!DashTarget.IsValid() || !StabAttackComponent || !GetOwner()) return;
	
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetOwner());
	if (!Enemy) return;
	
	// Face the target
	FVector ToTarget = (DashTarget->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
	FRotator NewRotation = ToTarget.Rotation();
	Enemy->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
	
	// Apply damage multiplier for backstab
	float OriginalDamage = StabAttackComponent->GetBaseDamage();
	StabAttackComponent->SetBaseDamage(OriginalDamage * BackstabDamageMultiplier);
	
	// Execute the stab
	StabAttackComponent->Execute();
	
	// Reset damage
	StabAttackComponent->SetBaseDamage(OriginalDamage);
	
	// Apply stagger to player if hit
	if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(DashTarget.Get()))
	{
		// Check if we're close enough for a successful backstab
		float Distance = FVector::Dist(Enemy->GetActorLocation(), Player->GetActorLocation());
		if (Distance <= StabAttackComponent->AttackRange)
		{
			Player->ApplyStagger(BackstabStaggerDuration);
			UE_LOG(LogTemp, Warning, TEXT("AssassinApproach: Backstab successful! Applied %.1fs stagger"), BackstabStaggerDuration);
		}
	}
	
	// Clear target reference
	DashTarget = nullptr;
}