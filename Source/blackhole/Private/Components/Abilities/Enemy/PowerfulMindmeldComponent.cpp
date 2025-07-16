#include "Components/Abilities/Enemy/PowerfulMindmeldComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "UI/BlackholeHUD.h"

UPowerfulMindmeldComponent::UPowerfulMindmeldComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	Cooldown = 60.0f; // Long cooldown
	Range = 3000.0f;
	WPCost = 0.0f; // No cost for enemy ability
	bIsBasicAbility = false;
	bIsChanneling = false;
	ChannelStartTime = 0.0f;
}

void UPowerfulMindmeldComponent::BeginPlay()
{
	Super::BeginPlay();
	Range = ChannelRange;
}

void UPowerfulMindmeldComponent::Execute()
{
	if (!CanExecute() || bIsChanneling)
	{
		return;
	}
	
	// Find the player
	ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	if (!Player) return;
	
	// Check range
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Player->GetActorLocation());
	if (Distance > ChannelRange)
	{
		return;
	}
	
	// Check line of sight if required
	if (bRequiresLineOfSight)
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		
		if (!GetWorld()->LineTraceSingleByChannel(HitResult, 
			GetOwner()->GetActorLocation() + FVector(0, 0, 50), 
			Player->GetActorLocation() + FVector(0, 0, 50),
			ECC_Visibility, QueryParams))
		{
			// Clear line of sight
			ChannelTarget = Player;
			Super::Execute();
			StartChannel();
		}
	}
	else
	{
		ChannelTarget = Player;
		Super::Execute();
		StartChannel();
	}
}

void UPowerfulMindmeldComponent::StartChannel()
{
	bIsChanneling = true;
	ChannelStartTime = GetWorld()->GetTimeSeconds();
	
	// Start completion timer
	GetWorld()->GetTimerManager().SetTimer(ChannelCompleteTimer, this, 
		&UPowerfulMindmeldComponent::CompleteChannel, CastTime, false);
	
	// Notify about channel start
	OnMindmeldStarted.Broadcast(CastTime);
	NotifyPlayerOfChannel();
	
	// Notify HUD
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ABlackholeHUD* HUD = Cast<ABlackholeHUD>(PC->GetHUD()))
		{
			HUD->OnMindmeldStarted(CastTime, GetOwner());
		}
	}
	
	// Visual effect
	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		// TODO: Play channeling animation/effect
	}
	
	UE_LOG(LogTemp, Error, TEXT("MIND MELDER: Starting %0.f second channel! Get close to interrupt!"), CastTime);
}

void UPowerfulMindmeldComponent::UpdateChannel()
{
	if (!bIsChanneling || !ChannelTarget) return;
	
	// Check if we should interrupt
	if (CheckInterruptConditions())
	{
		InterruptChannel();
		return;
	}
	
	// Visual feedback
	#if WITH_EDITOR
	FVector OwnerLoc = GetOwner()->GetActorLocation() + FVector(0, 0, 100);
	FVector TargetLoc = ChannelTarget->GetActorLocation() + FVector(0, 0, 100);
	
	// Draw channeling beam
	float Progress = GetChannelProgress();
	FColor BeamColor = FColor::MakeRedToGreenColorFromScalar(1.0f - Progress);
	DrawDebugLine(GetWorld(), OwnerLoc, TargetLoc, BeamColor, false, -1.0f, 0, 5.0f);
	
	// Draw progress circle
	DrawDebugCircle(GetWorld(), GetOwner()->GetActorLocation(), 100.0f * Progress, 32, 
		BeamColor, false, -1.0f, 0, 5.0f, FVector(0, 1, 0), FVector(1, 0, 0));
	#endif
}

void UPowerfulMindmeldComponent::CompleteChannel()
{
	if (!bIsChanneling || !ChannelTarget) return;
	
	// Drop player WP to 0
	if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(ChannelTarget))
	{
		if (UWillPowerComponent* WPComp = Player->FindComponentByClass<UWillPowerComponent>())
		{
			float CurrentWP = WPComp->GetCurrentValue();
			WPComp->DrainWillPower(CurrentWP); // Drop to 0
			
			UE_LOG(LogTemp, Error, TEXT("MIND MELD COMPLETE: Player WP dropped to 0!"));
		}
	}
	
	OnMindmeldComplete.Broadcast();
	
	// Notify HUD
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ABlackholeHUD* HUD = Cast<ABlackholeHUD>(PC->GetHUD()))
		{
			HUD->OnMindmeldComplete();
		}
	}
	
	bIsChanneling = false;
	ChannelTarget = nullptr;
	GetWorld()->GetTimerManager().ClearTimer(ChannelCompleteTimer);
}

void UPowerfulMindmeldComponent::InterruptChannel()
{
	if (!bIsChanneling) return;
	
	UE_LOG(LogTemp, Warning, TEXT("Mind meld interrupted!"));
	
	OnMindmeldInterrupted.Broadcast();
	
	// Notify HUD
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ABlackholeHUD* HUD = Cast<ABlackholeHUD>(PC->GetHUD()))
		{
			HUD->OnMindmeldInterrupted();
		}
	}
	
	bIsChanneling = false;
	ChannelTarget = nullptr;
	GetWorld()->GetTimerManager().ClearTimer(ChannelCompleteTimer);
	
	// Put ability on cooldown
	StartCooldown();
}

void UPowerfulMindmeldComponent::Deactivate()
{
	if (bIsChanneling)
	{
		InterruptChannel();
	}
	
	Super::Deactivate();
}

void UPowerfulMindmeldComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsChanneling)
	{
		UpdateChannel();
	}
}

float UPowerfulMindmeldComponent::GetChannelProgress() const
{
	if (!bIsChanneling) return 0.0f;
	
	float ElapsedTime = GetWorld()->GetTimeSeconds() - ChannelStartTime;
	return FMath::Clamp(ElapsedTime / CastTime, 0.0f, 1.0f);
}

float UPowerfulMindmeldComponent::GetTimeRemaining() const
{
	if (!bIsChanneling) return 0.0f;
	
	float ElapsedTime = GetWorld()->GetTimeSeconds() - ChannelStartTime;
	return FMath::Max(0.0f, CastTime - ElapsedTime);
}

bool UPowerfulMindmeldComponent::CheckInterruptConditions()
{
	if (!ChannelTarget || !GetOwner()) return true;
	
	// Check if player is close enough to interrupt
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), ChannelTarget->GetActorLocation());
	if (Distance <= InterruptRange)
	{
		// Check if player damaged the caster
		if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
		{
			// This would be triggered by damage events
			return true;
		}
	}
	
	// Check if lost line of sight
	if (bRequiresLineOfSight)
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		
		if (GetWorld()->LineTraceSingleByChannel(HitResult, 
			GetOwner()->GetActorLocation() + FVector(0, 0, 50), 
			ChannelTarget->GetActorLocation() + FVector(0, 0, 50),
			ECC_Visibility, QueryParams))
		{
			// Line of sight blocked
			return true;
		}
	}
	
	// Check if target is still valid
	if (!IsValid(ChannelTarget))
	{
		return true;
	}
	
	return false;
}

void UPowerfulMindmeldComponent::NotifyPlayerOfChannel()
{
	// Get player controller
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		// Show location indicator
		FVector EnemyLocation = GetOwner()->GetActorLocation();
		
		UE_LOG(LogTemp, Error, TEXT("WARNING: MIND MELDER channeling at location %s! %.0f seconds to interrupt!"), 
			*EnemyLocation.ToString(), CastTime);
		
		// Draw debug sphere at enemy location
		#if WITH_EDITOR
		DrawDebugSphere(GetWorld(), EnemyLocation, 200.0f, 32, FColor::Red, false, CastTime, 0, 10.0f);
		#endif
		
		// HUD notifications are now handled in StartChannel()
	}
}