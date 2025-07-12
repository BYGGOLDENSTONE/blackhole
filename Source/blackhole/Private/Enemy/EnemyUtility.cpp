#include "Enemy/EnemyUtility.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Config/GameplayConfig.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"

void UEnemyUtility::MoveTowardsTarget(ABaseEnemy* Enemy, AActor* Target, float DeltaTime, float MoveSpeed)
{
	if (!Enemy || !Target || Enemy->IsDead())
	{
		return;
	}
	
	// Get direction to target
	FVector Direction = GetDirectionToTarget(Enemy, Target);
	
	// Move the enemy
	Enemy->AddMovementInput(Direction, MoveSpeed * DeltaTime);
	
	// Face the target
	if (Enemy->GetCharacterMovement() && !Direction.IsNearlyZero())
	{
		FRotator NewRotation = Direction.Rotation();
		NewRotation.Pitch = 0.0f;
		NewRotation.Roll = 0.0f;
		Enemy->SetActorRotation(NewRotation);
	}
}

float UEnemyUtility::GetDistanceToTarget(const ABaseEnemy* Enemy, const AActor* Target)
{
	if (!Enemy || !Target)
	{
		return MAX_FLT;
	}
	
	return FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
}

FVector UEnemyUtility::GetDirectionToTarget(const ABaseEnemy* Enemy, const AActor* Target)
{
	if (!Enemy || !Target)
	{
		return FVector::ZeroVector;
	}
	
	FVector Direction = Target->GetActorLocation() - Enemy->GetActorLocation();
	Direction.Z = 0.0f; // Keep movement on horizontal plane
	Direction.Normalize();
	
	return Direction;
}

bool UEnemyUtility::TryBasicAttack(ABaseEnemy* Enemy, float AttackRange, float Damage)
{
	if (!Enemy || Enemy->IsDead())
	{
		return false;
	}
	
	if (!IsInAttackRange(Enemy, AttackRange))
	{
		return false;
	}
	
	// Get player reference
	ABlackholePlayerCharacter* Player = Enemy->GetTargetPlayer();
	if (!Player)
	{
		return false;
	}
	
	// Apply damage
	FPointDamageEvent DamageEvent(Damage, FHitResult(), Enemy->GetActorLocation(), nullptr);
	Player->TakeDamage(Damage, DamageEvent, Enemy->GetController(), Enemy);
	
	UE_LOG(LogTemp, Verbose, TEXT("%s attacked player for %.1f damage"), *Enemy->GetName(), Damage);
	
	return true;
}

bool UEnemyUtility::IsInAttackRange(const ABaseEnemy* Enemy, float AttackRange)
{
	if (!Enemy)
	{
		return false;
	}
	
	const ABlackholePlayerCharacter* Player = Enemy->GetTargetPlayer();
	if (!Player)
	{
		return false;
	}
	
	return GetDistanceToTarget(Enemy, Player) <= AttackRange;
}

bool UEnemyUtility::HasLineOfSightToPlayer(const ABaseEnemy* Enemy)
{
	if (!Enemy)
	{
		return false;
	}
	
	const ABlackholePlayerCharacter* Player = Enemy->GetTargetPlayer();
	if (!Player)
	{
		return false;
	}
	
	FVector StartLocation = Enemy->GetActorLocation() + FVector(0, 0, GameplayConfig::Enemy::SIGHT_HEIGHT_OFFSET);
	FVector EndLocation = Player->GetActorLocation();
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Enemy);
	QueryParams.AddIgnoredActor(Player);
	
	return !Enemy->GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		QueryParams
	);
}

ABlackholePlayerCharacter* UEnemyUtility::FindPlayerInWorld(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	
	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}
	
	// Find the player character
	for (TActorIterator<ABlackholePlayerCharacter> It(World); It; ++It)
	{
		if (ABlackholePlayerCharacter* Player = *It)
		{
			return Player;
		}
	}
	
	return nullptr;
}

bool UEnemyUtility::IsPlayerInDetectionRange(const ABaseEnemy* Enemy, float DetectionRange)
{
	if (!Enemy)
	{
		return false;
	}
	
	const ABlackholePlayerCharacter* Player = Enemy->GetTargetPlayer();
	if (!Player)
	{
		return false;
	}
	
	return GetDistanceToTarget(Enemy, Player) <= DetectionRange;
}

void UEnemyUtility::StartCombatState(ABaseEnemy* Enemy)
{
	if (!Enemy || Enemy->IsDead())
	{
		return;
	}
	
	if (!Enemy->HasStartedCombat())
	{
		Enemy->SetCombatStarted(true);
		Enemy->OnCombatStart();
		
		UE_LOG(LogTemp, Log, TEXT("%s entered combat"), *Enemy->GetName());
	}
}

void UEnemyUtility::EndCombatState(ABaseEnemy* Enemy)
{
	if (!Enemy || Enemy->IsDead())
	{
		return;
	}
	
	if (Enemy->HasStartedCombat())
	{
		Enemy->SetCombatStarted(false);
		Enemy->OnCombatEnd();
		
		UE_LOG(LogTemp, Log, TEXT("%s left combat"), *Enemy->GetName());
	}
}