#include "Components/Abilities/Player/Utility/ForgeJumpAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Components/CapsuleComponent.h"
#include "Particles/ParticleSystem.h"

UForgeJumpAbility::UForgeJumpAbility()
{
	// Enable ticking for slam detection
	PrimaryComponentTick.bCanEverTick = true;
	
	// Set forge path
	PathType = ECharacterPath::Forge;
	
	// Jump costs and cooldown
	Cost = 10.0f; // Legacy field
	StaminaCost = 10.0f; // New dual resource system
	WPCost = 0.0f; // Utility abilities don't consume WP
	HeatCost = 0.0f; // New dual resource system
	Cooldown = 3.0f;
	
	// Jump specifics
	JumpVelocity = 800.0f;
	SlamVelocity = -2000.0f;
	SlamDamage = 20.0f;
	SlamRadius = 300.0f;
	MinHeightForSlam = 200.0f;
}

void UForgeJumpAbility::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind to landing event
	if (ACharacter* Character = GetCharacterOwner())
	{
		Character->LandedDelegate.AddDynamic(this, &UForgeJumpAbility::OnLanded);
	}
}

void UForgeJumpAbility::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Check if we should enter slam mode
	if (bIsInSlamMode && !bIsSlamming && CanInitiateSlam())
	{
		// Player has reached sufficient height, allow slam on next input
		// Visual feedback could be added here
		
		#if WITH_EDITOR
		if (ACharacter* Character = GetCharacterOwner())
		{
			DrawDebugString(GetWorld(), Character->GetActorLocation(), TEXT("SLAM READY!"), nullptr, FColor::Orange, 0.0f);
		}
		#endif
	}
}

bool UForgeJumpAbility::CanExecute() const
{
	// During slam mode, we can execute to trigger the slam
	if (bIsInSlamMode)
	{
		return CanInitiateSlam();
	}
	
	// Otherwise, check normal execution conditions
	return Super::CanExecute() && CachedMovement && CachedMovement->IsMovingOnGround();
}

void UForgeJumpAbility::ApplyMovement(ACharacter* Character)
{
	if (!Character || !CachedMovement)
	{
		return;
	}
	
	// If we're in slam mode and high enough, initiate slam
	if (bIsInSlamMode && CanInitiateSlam())
	{
		bIsSlamming = true;
		
		// Apply downward slam velocity
		CachedMovement->Velocity = FVector(0.0f, 0.0f, SlamVelocity);
		
		// Disable air control during slam
		CachedMovement->AirControl = 0.0f;
		
		#if WITH_EDITOR
		FVector CharLocation = Character->GetActorLocation();
		DrawDebugLine(GetWorld(), CharLocation, CharLocation + FVector(0, 0, -1000), FColor::Red, false, 2.0f, 0, 5.0f);
		#endif
		
		return;
	}
	
	// Otherwise, perform normal jump
	if (!bIsInSlamMode && CachedMovement->IsMovingOnGround())
	{
		// Store jump start height
		JumpStartHeight = Character->GetActorLocation().Z;
		
		// Enter slam mode
		bIsInSlamMode = true;
		
		// Use character's built-in jump for consistency
		Character->Jump();
		
		// Override the jump velocity if needed
		if (CachedMovement->Velocity.Z < JumpVelocity)
		{
			CachedMovement->Velocity.Z = JumpVelocity;
		}
		
		// Slightly reduced air control for forge jump
		CachedMovement->AirControl *= 0.7f;
		
		#if WITH_EDITOR
		FVector CharLocation = Character->GetActorLocation();
		DrawDebugLine(GetWorld(), CharLocation, CharLocation + FVector(0, 0, JumpVelocity), FColor::Blue, false, 1.0f, 0, 3.0f);
		#endif
	}
}

void UForgeJumpAbility::OnLanded(const FHitResult& Hit)
{
	ACharacter* Character = GetCharacterOwner();
	if (!Character)
	{
		return;
	}
	
	// If we were slamming, apply damage
	if (bIsSlamming)
	{
		ApplySlamDamage(Character->GetActorLocation());
		
		// Camera shake effect
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			// TODO: Replace with specific camera shake blueprint class
			// PC->ClientStartCameraShake(YourCameraShakeClass, 1.0f);
		}
	}
	
	// Reset slam states
	bIsInSlamMode = false;
	bIsSlamming = false;
	JumpStartHeight = 0.0f;
	
	// Restore air control
	if (CachedMovement)
	{
		CachedMovement->AirControl = 0.2f; // Default UE4 value
	}
}

void UForgeJumpAbility::ApplySlamDamage(const FVector& ImpactLocation)
{
	// Get all actors in slam radius
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	
	GetWorld()->SweepMultiByChannel(
		HitResults,
		ImpactLocation,
		ImpactLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(SlamRadius),
		QueryParams
	);
	
	// Apply damage and knockback to enemies
	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || (!HitActor->ActorHasTag("Enemy") && !HitActor->IsA<ABaseEnemy>()))
		{
			continue;
		}
		
		// Calculate distance-based damage falloff
		float Distance = FVector::Dist(HitActor->GetActorLocation(), ImpactLocation);
		float DamageFalloff = 1.0f - (Distance / SlamRadius);
		float FinalDamage = SlamDamage * DamageFalloff;
		
		// Apply damage
		if (UIntegrityComponent* TargetIntegrity = HitActor->FindComponentByClass<UIntegrityComponent>())
		{
			TargetIntegrity->TakeDamage(FinalDamage);
		}
		
		// Apply radial knockback
		if (ACharacter* EnemyCharacter = Cast<ACharacter>(HitActor))
		{
			FVector KnockbackDirection = (HitActor->GetActorLocation() - ImpactLocation).GetSafeNormal();
			KnockbackDirection.Z = 0.5f; // Strong upward component
			
			if (UCharacterMovementComponent* EnemyMovement = EnemyCharacter->GetCharacterMovement())
			{
				EnemyMovement->AddImpulse(KnockbackDirection * 800.0f * DamageFalloff, true);
			}
		}
	}
	
	#if WITH_EDITOR
	// Debug visualization of slam impact
	DrawDebugSphere(GetWorld(), ImpactLocation, SlamRadius, 32, FColor::Red, false, 2.0f);
	
	// Draw shockwave rings
	for (int i = 1; i <= 3; i++)
	{
		float RingRadius = (SlamRadius / 3.0f) * i;
		DrawDebugCircle(GetWorld(), ImpactLocation, RingRadius, 32, FColor::Orange, false, 2.0f, 0, 2.0f, FVector(0, 1, 0), FVector(1, 0, 0));
	}
	#endif
}

bool UForgeJumpAbility::CanInitiateSlam() const
{
	ACharacter* Character = GetCharacterOwner();
	if (!Character || !CachedMovement)
	{
		return false;
	}
	
	// Must be falling
	if (!CachedMovement->IsFalling())
	{
		return false;
	}
	
	// Check if we're high enough
	float CurrentHeight = Character->GetActorLocation().Z;
	float HeightDifference = CurrentHeight - JumpStartHeight;
	
	return HeightDifference >= MinHeightForSlam;
}