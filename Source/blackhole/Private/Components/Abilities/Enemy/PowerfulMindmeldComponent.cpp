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
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"

UPowerfulMindmeldComponent::UPowerfulMindmeldComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	Cooldown = 45.0f; // Reduced cooldown for more frequent use
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

void UPowerfulMindmeldComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Interrupt channel if owner is being destroyed
	if (bIsChanneling)
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerfulMindmeld: MindMelder destroyed, interrupting channel!"));
		InterruptChannel();
	}
	
	Super::EndPlay(EndPlayReason);
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
		
		bool bHitSomething = GetWorld()->LineTraceSingleByChannel(HitResult, 
			GetOwner()->GetActorLocation() + FVector(0, 0, 50), 
			Player->GetActorLocation() + FVector(0, 0, 50),
			ECC_Visibility, QueryParams);
		
		// Check if we hit the player or nothing (clear line of sight)
		if (!bHitSomething || HitResult.GetActor() == Player)
		{
			// Clear line of sight
			ChannelTarget = Player;
			Super::Execute();
			StartChannel();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PowerfulMindmeld: No line of sight to player"));
			return;
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
	
	UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: Starting channel! CastTime=%.1f, Target=%s"), 
		CastTime, ChannelTarget ? *ChannelTarget->GetName() : TEXT("nullptr"));
	
	// Start completion timer
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda([this]()
	{
		UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: Timer fired! Calling CompleteChannel..."));
		CompleteChannel();
	});
	
	GetWorld()->GetTimerManager().SetTimer(ChannelCompleteTimer, TimerDelegate, CastTime, false);
	
	// Verify timer was set
	if (GetWorld()->GetTimerManager().IsTimerActive(ChannelCompleteTimer))
	{
		float TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(ChannelCompleteTimer);
		UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: Timer successfully set! CastTime=%.1f, TimeRemaining=%.1f"), CastTime, TimeRemaining);
		
		// Set a recurring log timer to track progress
		FTimerHandle LogTimer;
		GetWorld()->GetTimerManager().SetTimer(LogTimer, [this]()
		{
			if (bIsChanneling)
			{
				float Remaining = GetWorld()->GetTimerManager().GetTimerRemaining(ChannelCompleteTimer);
				UE_LOG(LogTemp, Warning, TEXT("PowerfulMindmeld: Channel progress - %.1f seconds remaining"), Remaining);
			}
		}, 5.0f, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: FAILED to set timer!"));
	}
	
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
	
	UE_LOG(LogTemp, Error, TEXT("MIND MELDER: Starting %.0f second channel! Only death can interrupt!"), CastTime);
}

void UPowerfulMindmeldComponent::UpdateChannel()
{
	if (!bIsChanneling) 
	{
		return;
	}
	
	if (!ChannelTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerfulMindmeld: UpdateChannel - Lost target during channel!"));
		InterruptChannel();
		return;
	}
	
	// Check if we should interrupt (only if owner dies)
	if (CheckInterruptConditions())
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerfulMindmeld: UpdateChannel - Interrupt conditions met (owner died)!"));
		InterruptChannel();
		return;
	}
	
	// Log progress
	float TimeRemaining = GetTimeRemaining();
	if (FMath::Fmod(TimeRemaining, 5.0f) < 0.2f) // Log every 5 seconds
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerfulMindmeld: Channel in progress - %.1f seconds remaining"), TimeRemaining);
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
	UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: CompleteChannel called! bIsChanneling=%d, ChannelTarget=%s"), 
		bIsChanneling, ChannelTarget ? *ChannelTarget->GetName() : TEXT("nullptr"));
	
	if (!bIsChanneling || !ChannelTarget) return;
	
	// Drop player WP to 0
	if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(ChannelTarget))
	{
		UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: Found player %s"), *Player->GetName());
		
		// Get ResourceManager to properly drain WP and trigger critical state
		if (UResourceManager* ResMgr = GetWorld()->GetGameInstance()->GetSubsystem<UResourceManager>())
		{
			float CurrentWP = ResMgr->GetCurrentWillPower();
			float MaxWP = ResMgr->GetMaxWillPower();
			UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: Player current WP = %.1f, draining all via ResourceManager..."), CurrentWP);
			
			// Use TakeDamage to drain all WP - this will trigger OnWPDepleted and start critical timer
			ResMgr->TakeDamage(CurrentWP);
			
			// Verify it worked
			float NewWP = ResMgr->GetCurrentWillPower();
			UE_LOG(LogTemp, Error, TEXT("MIND MELD COMPLETE: Player WP dropped from %.1f to %.1f!"), CurrentWP, NewWP);
			
			// Force a special MindMeld critical state that ignores entries
			if (UThresholdManager* ThreshMgr = GetWorld()->GetSubsystem<UThresholdManager>())
			{
				// Notify ThresholdManager that this is a MindMeld-induced critical state
				// This should force ultimate usage, not just consume an entry
				UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: Forcing MindMeld critical state - player MUST use ultimate!"));
				ThreshMgr->StartCriticalTimer();
			}
		}
		else if (UWillPowerComponent* WPComp = Player->FindComponentByClass<UWillPowerComponent>())
		{
			// Fallback to direct component access
			float CurrentWP = WPComp->GetCurrentValue();
			UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: No ResourceManager, using WillPowerComponent directly"));
			WPComp->SetCurrentValue(0.0f);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: Could not find WillPowerComponent on player!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PowerfulMindmeld: Failed to cast ChannelTarget to player!"));
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
	
	// Only interrupt if owner is dead or target is invalid
	// Do NOT interrupt for proximity or line of sight
	
	// Check if owner (MindMelder) is still alive
	if (!IsValid(GetOwner()))
	{
		return true;
	}
	
	// Check if target is still valid
	if (!IsValid(ChannelTarget))
	{
		return true;
	}
	
	// Channel continues unless MindMelder is killed
	return false;
}

void UPowerfulMindmeldComponent::NotifyPlayerOfChannel()
{
	// Get player controller
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		// Show location indicator
		FVector EnemyLocation = GetOwner()->GetActorLocation();
		
		UE_LOG(LogTemp, Error, TEXT("WARNING: MIND MELDER channeling PowerfulMindmeld! %.0f seconds until WP drops to 0! Kill the MindMelder to interrupt!"), 
			CastTime);
		
		// Draw debug sphere at enemy location
		#if WITH_EDITOR
		DrawDebugSphere(GetWorld(), EnemyLocation, 200.0f, 32, FColor::Red, false, CastTime, 0, 10.0f);
		#endif
		
		// HUD notifications are now handled in StartChannel()
	}
}