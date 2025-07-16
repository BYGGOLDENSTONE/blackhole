#include "Components/Abilities/Enemy/BuilderComponent.h"
#include "Enemy/StandardEnemy.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Actors/PsiDisruptor.h"
#include "UI/BlackholeHUD.h"

// Static member initialization
TArray<UBuilderComponent*> UBuilderComponent::AllActiveBuilders;

UBuilderComponent::UBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	bIsBuilding = false;
	bIsBuildLeader = false;
	BuildProgress = 0.0f;
	LeaderComponent = nullptr;
}

void UBuilderComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Register this builder
	AllActiveBuilders.Add(this);
}

void UBuilderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister this builder
	AllActiveBuilders.Remove(this);
	
	// Cancel any ongoing build
	if (bIsBuilding)
	{
		CancelBuild();
	}
	
	Super::EndPlay(EndPlayReason);
}

void UBuilderComponent::InitiateBuild(const FVector& BuildLocation)
{
	if (bIsBuilding) return;
	
	CurrentBuildLocation = BuildLocation;
	bIsBuilding = true;
	bIsBuildLeader = true;
	BuildProgress = 0.0f;
	LeaderComponent = this;
	ParticipatingBuilders.Empty();
	ParticipatingBuilders.Add(this);
	
	// Notify nearby builders to join
	for (UBuilderComponent* Builder : AllActiveBuilders)
	{
		if (Builder != this && !Builder->IsBuilding())
		{
			float Distance = FVector::Dist(Builder->GetOwner()->GetActorLocation(), BuildLocation);
			if (Distance <= BuildRadius)
			{
				Builder->JoinBuild(this);
			}
		}
	}
	
	// Check if we have enough builders
	if (ParticipatingBuilders.Num() >= MinBuildersRequired)
	{
		SetComponentTickEnabled(true);
		OnBuildingStarted.Broadcast(BuildLocation);
		
		// Notify HUD
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (ABlackholeHUD* HUD = Cast<ABlackholeHUD>(PC->GetHUD()))
			{
				HUD->OnPsiDisruptorBuildStarted(BuildLocation);
			}
		}
		
		// Start build timer
		GetWorld()->GetTimerManager().SetTimer(BuildTimerHandle, this, &UBuilderComponent::UpdateBuildProgress, 0.1f, true);
		
		UE_LOG(LogTemp, Warning, TEXT("Building started with %d builders at location %s"), 
			ParticipatingBuilders.Num(), *BuildLocation.ToString());
	}
	else
	{
		// Not enough builders
		CancelBuild();
		UE_LOG(LogTemp, Warning, TEXT("Not enough builders to start build. Required: %d, Have: %d"), 
			MinBuildersRequired, ParticipatingBuilders.Num());
	}
}

void UBuilderComponent::JoinBuild(UBuilderComponent* Leader)
{
	if (bIsBuilding || !Leader || !Leader->IsBuilding()) return;
	
	bIsBuilding = true;
	bIsBuildLeader = false;
	LeaderComponent = Leader;
	CurrentBuildLocation = Leader->GetBuildLocation();
	
	// Add to leader's participant list
	Leader->ParticipatingBuilders.Add(this);
	
	SetComponentTickEnabled(true);
}

void UBuilderComponent::CancelBuild()
{
	if (!bIsBuilding) return;
	
	// Clear timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BuildTimerHandle);
	}
	
	// If we're the leader, notify all participants
	if (bIsBuildLeader)
	{
		for (UBuilderComponent* Builder : ParticipatingBuilders)
		{
			if (Builder && Builder != this)
			{
				Builder->CancelBuild();
			}
		}
		
		OnBuildingCancelled.Broadcast();
	}
	
	// Reset state
	bIsBuilding = false;
	bIsBuildLeader = false;
	BuildProgress = 0.0f;
	LeaderComponent = nullptr;
	ParticipatingBuilders.Empty();
	SetComponentTickEnabled(false);
	
	UE_LOG(LogTemp, Warning, TEXT("Build cancelled"));
}

float UBuilderComponent::GetBuildProgress() const
{
	if (bIsBuildLeader)
	{
		return BuildProgress;
	}
	else if (LeaderComponent)
	{
		return LeaderComponent->GetBuildProgress();
	}
	
	return 0.0f;
}

void UBuilderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!bIsBuilding) return;
	
	// Visual feedback - draw progress circle
	#if WITH_EDITOR
	if (bIsBuildLeader)
	{
		float Radius = 100.0f * BuildProgress;
		DrawDebugCircle(GetWorld(), CurrentBuildLocation, Radius, 32, FColor::Blue, false, -1.0f, 0, 2.0f);
	}
	#endif
	
	// Check if builders are still alive and in range
	if (bIsBuildLeader)
	{
		TArray<UBuilderComponent*> ValidBuilders;
		for (UBuilderComponent* Builder : ParticipatingBuilders)
		{
			if (Builder && IsValid(Builder->GetOwner()))
			{
				float Distance = FVector::Dist(Builder->GetOwner()->GetActorLocation(), CurrentBuildLocation);
				if (Distance <= BuildRadius * 1.5f)
				{
					ValidBuilders.Add(Builder);
				}
			}
		}
		
		ParticipatingBuilders = ValidBuilders;
		
		// Cancel if not enough builders
		if (ParticipatingBuilders.Num() < MinBuildersRequired)
		{
			CancelBuild();
		}
	}
}

void UBuilderComponent::UpdateBuildProgress()
{
	if (!bIsBuildLeader || !bIsBuilding) return;
	
	// Progress based on number of builders
	float ProgressRate = 1.0f / BuildTime; // Base rate
	float BuilderMultiplier = FMath::Clamp(ParticipatingBuilders.Num() / (float)MinBuildersRequired, 1.0f, 2.0f);
	
	BuildProgress += ProgressRate * 0.1f * BuilderMultiplier; // 0.1f is timer interval
	
	if (BuildProgress >= 1.0f)
	{
		CompleteBuild();
	}
}

void UBuilderComponent::CompleteBuild()
{
	if (!bIsBuildLeader) return;
	
	SpawnPsiDisruptor();
	
	// Notify all participants
	for (UBuilderComponent* Builder : ParticipatingBuilders)
	{
		if (Builder)
		{
			Builder->bIsBuilding = false;
			Builder->SetComponentTickEnabled(false);
		}
	}
	
	OnBuildingComplete.Broadcast();
	
	// Notify HUD
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ABlackholeHUD* HUD = Cast<ABlackholeHUD>(PC->GetHUD()))
		{
			HUD->OnPsiDisruptorBuildComplete();
		}
	}
	
	// Reset leader state
	CancelBuild();
}

void UBuilderComponent::SpawnPsiDisruptor()
{
	if (!PsiDisruptorClass || !GetWorld()) return;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	FVector SpawnLocation = CurrentBuildLocation + FVector(0, 0, 50); // Slight offset
	FRotator SpawnRotation = FRotator::ZeroRotator;
	
	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(PsiDisruptorClass, SpawnLocation, SpawnRotation, SpawnParams);
	SpawnedDisruptor = Cast<APsiDisruptor>(SpawnedActor);
	
	if (SpawnedDisruptor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Psi-Disruptor spawned successfully at %s"), *SpawnLocation.ToString());
	}
}

UBuilderComponent* UBuilderComponent::FindNearestBuildLeader(AActor* SearchOrigin, float MaxRange)
{
	if (!SearchOrigin) return nullptr;
	
	UBuilderComponent* NearestLeader = nullptr;
	float NearestDistance = MaxRange;
	
	for (UBuilderComponent* Builder : AllActiveBuilders)
	{
		if (Builder && Builder->IsBuilding() && Builder->IsLeader())
		{
			float Distance = FVector::Dist(SearchOrigin->GetActorLocation(), Builder->GetBuildLocation());
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				NearestLeader = Builder;
			}
		}
	}
	
	return NearestLeader;
}

TArray<UBuilderComponent*> UBuilderComponent::GetAllActiveBuilders()
{
	return AllActiveBuilders;
}