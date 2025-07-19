#include "Components/Abilities/Enemy/BuilderComponent.h"
#include "Enemy/StandardEnemy.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Actors/PsiDisruptor.h"
#include "UI/BlackholeHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Enemy/AI/EnemyStateMachine.h"

// Static member initialization
TArray<UBuilderComponent*> UBuilderComponent::AllActiveBuilders;

UBuilderComponent::UBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	bIsBuilding = false;
	bIsBuildLeader = false;
	bBuildPaused = false;
	BuildProgress = 0.0f;
	CurrentBuildTime = BuildTime;
	TimeSpentBuilding = 0.0f;
	LeaderComponent = nullptr;
	InitialBuilderCount = 0;
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
	bBuildPaused = false;
	BuildProgress = 0.0f;
	CurrentBuildTime = BuildTime;
	TimeSpentBuilding = 0.0f;
	LeaderComponent = this;
	ParticipatingBuilders.Empty();
	ParticipatingBuilders.Add(this);
	
	// Create visual build sphere
	CreateBuildSphere();
	
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
		// Store initial builder count for timer calculations
		InitialBuilderCount = ParticipatingBuilders.Num();
		
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
		
		// Notify owner to enter building state
		if (AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(GetOwner()))
		{
			if (UEnemyStateMachine* StateMachine = StandardEnemy->GetStateMachine())
			{
				StateMachine->ChangeState(EEnemyState::Building);
			}
		}
		
		// UE_LOG(LogTemp, Warning, TEXT("Building started with %d builders at location %s"), 
		//	ParticipatingBuilders.Num(), *BuildLocation.ToString());
	}
	else
	{
		// Not enough builders - destroy sphere and cancel
		DestroyBuildSphere();
		CancelBuild();
		// UE_LOG(LogTemp, Warning, TEXT("Not enough builders to start build. Required: %d, Have: %d"), 
		//	MinBuildersRequired, ParticipatingBuilders.Num());
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
	
	// Notify owner to enter building state
	if (AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(GetOwner()))
	{
		if (UEnemyStateMachine* StateMachine = StandardEnemy->GetStateMachine())
		{
			StateMachine->ChangeState(EEnemyState::Building);
		}
	}
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
		
		// Destroy visual sphere
		DestroyBuildSphere();
		
		OnBuildingCancelled.Broadcast();
	}
	
	// Notify owner to exit building state
	if (AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(GetOwner()))
	{
		if (UEnemyStateMachine* StateMachine = StandardEnemy->GetStateMachine())
		{
			// Return to idle state after cancelling build
			if (StateMachine->GetCurrentState() == EEnemyState::Building)
			{
				StateMachine->ChangeState(EEnemyState::Idle);
			}
		}
	}
	
	// Reset state
	bIsBuilding = false;
	bIsBuildLeader = false;
	bBuildPaused = false;
	BuildProgress = 0.0f;
	CurrentBuildTime = BuildTime;
	TimeSpentBuilding = 0.0f;
	LeaderComponent = nullptr;
	ParticipatingBuilders.Empty();
	InitialBuilderCount = 0;
	SetComponentTickEnabled(false);
	
	// UE_LOG(LogTemp, Warning, TEXT("Build cancelled"));
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
	if (!bIsBuildLeader || !bIsBuilding || bBuildPaused) return;
	
	// Check if all participating builders are still alive
	TArray<UBuilderComponent*> AliveBuilders;
	for (UBuilderComponent* Builder : ParticipatingBuilders)
	{
		if (Builder && IsValid(Builder) && IsValid(Builder->GetOwner()))
		{
			AliveBuilders.Add(Builder);
		}
	}
	
	// Update participating builders list
	ParticipatingBuilders = AliveBuilders;
	
	// Check if we still have builders
	if (ParticipatingBuilders.Num() == 0)
	{
		// All builders are dead - pause the build
		// UE_LOG(LogTemp, Warning, TEXT("All builders dead - pausing build at %.1f%% progress"), 
		//	BuildProgress * 100.0f);
		PauseBuild();
		return;
	}
	
	// Check if a builder was killed and adjust timer
	if (ParticipatingBuilders.Num() < InitialBuilderCount)
	{
		// Calculate time remaining based on current progress
		float TimeRemaining = CurrentBuildTime - TimeSpentBuilding;
		
		// Double the remaining time for each builder killed
		int32 BuildersKilled = InitialBuilderCount - ParticipatingBuilders.Num();
		float NewTimeRemaining = TimeRemaining * FMath::Pow(1.5f, BuildersKilled); // 1.5x multiplier per death
		
		// Update current build time
		CurrentBuildTime = TimeSpentBuilding + NewTimeRemaining;
		
		// Update initial builder count to prevent repeated adjustments
		InitialBuilderCount = ParticipatingBuilders.Num();
		
		// UE_LOG(LogTemp, Warning, TEXT("Builder killed! Adjusting timer - was %.1fs remaining, now %.1fs (total build time: %.1fs)"), 
		//	TimeRemaining, NewTimeRemaining, CurrentBuildTime);
	}
	
	// Update time spent building
	TimeSpentBuilding += 0.1f; // 0.1f is timer interval
	
	// Calculate progress based on adjusted build time
	BuildProgress = TimeSpentBuilding / CurrentBuildTime;
	
	UE_LOG(LogTemp, Verbose, TEXT("Build progress: %.1f%% with %d builders"), 
		BuildProgress * 100.0f, ParticipatingBuilders.Num());
	
	if (BuildProgress >= 1.0f)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Build progress reached 100%%! Calling CompleteBuild..."));
		CompleteBuild();
	}
}

void UBuilderComponent::CompleteBuild()
{
	// UE_LOG(LogTemp, Warning, TEXT("CompleteBuild called! bIsBuildLeader = %s"), bIsBuildLeader ? TEXT("true") : TEXT("false"));
	
	if (!bIsBuildLeader) 
	{
		UE_LOG(LogTemp, Error, TEXT("CompleteBuild: Not the build leader, aborting!"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Build completed! Spawning Psi-Disruptor..."));
	
	SpawnPsiDisruptor();
	
	// Notify all participants
	for (UBuilderComponent* Builder : ParticipatingBuilders)
	{
		if (Builder)
		{
			Builder->bIsBuilding = false;
			Builder->SetComponentTickEnabled(false);
			
			// Notify to exit building state
			if (AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(Builder->GetOwner()))
			{
				if (UEnemyStateMachine* StateMachine = StandardEnemy->GetStateMachine())
				{
					if (StateMachine->GetCurrentState() == EEnemyState::Building)
					{
						StateMachine->ChangeState(EEnemyState::Idle);
					}
				}
			}
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
	if (!PsiDisruptorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnPsiDisruptor FAILED: PsiDisruptorClass is not set! Please set it in the Blueprint."));
		return;
	}
	
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnPsiDisruptor FAILED: No valid world context!"));
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	FVector SpawnLocation = CurrentBuildLocation + FVector(0, 0, 50); // Slight offset
	FRotator SpawnRotation = FRotator::ZeroRotator;
	
	// UE_LOG(LogTemp, Warning, TEXT("Attempting to spawn PsiDisruptor at %s..."), *SpawnLocation.ToString());
	
	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(PsiDisruptorClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (!SpawnedActor)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnPsiDisruptor FAILED: SpawnActor returned null!"));
		return;
	}
	
	SpawnedDisruptor = Cast<APsiDisruptor>(SpawnedActor);
	
	if (SpawnedDisruptor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Psi-Disruptor spawned successfully at %s"), *SpawnLocation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnPsiDisruptor: Actor spawned but is not a PsiDisruptor class!"));
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

void UBuilderComponent::PauseBuild()
{
	if (!bIsBuildLeader || !bIsBuilding || bBuildPaused) return;
	
	bBuildPaused = true;
	
	// Clear the timer but keep all state
	GetWorld()->GetTimerManager().ClearTimer(BuildTimerHandle);
	
	// Notify all builders to exit building state
	for (UBuilderComponent* Builder : ParticipatingBuilders)
	{
		if (Builder && IsValid(Builder->GetOwner()))
		{
			if (AStandardEnemy* StandardEnemy = Cast<AStandardEnemy>(Builder->GetOwner()))
			{
				if (UEnemyStateMachine* StateMachine = StandardEnemy->GetStateMachine())
				{
					if (StateMachine->GetCurrentState() == EEnemyState::Building)
					{
						StateMachine->ChangeState(EEnemyState::Combat);
					}
				}
			}
		}
	}
	
	// UE_LOG(LogTemp, Warning, TEXT("Build paused at %.1f%% progress (%.1fs spent, %.1fs total)"), 
	//	BuildProgress * 100.0f, TimeSpentBuilding, CurrentBuildTime);
}

void UBuilderComponent::ResumeBuild()
{
	if (!bIsBuildLeader || !bIsBuilding || !bBuildPaused) return;
	
	bBuildPaused = false;
	
	// Restart the timer
	GetWorld()->GetTimerManager().SetTimer(BuildTimerHandle, this, &UBuilderComponent::UpdateBuildProgress, 0.1f, true);
	
	// UE_LOG(LogTemp, Warning, TEXT("Build resumed at %.1f%% progress"), BuildProgress * 100.0f);
}

void UBuilderComponent::CreateBuildSphere()
{
	if (!bIsBuildLeader || !GetWorld()) return;
	
	// Create a simple actor to hold the sphere component
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	BuildSphereActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), CurrentBuildLocation, FRotator::ZeroRotator, SpawnParams);
	if (!BuildSphereActor) return;
	
	// Create the sphere mesh component
	BuildSphere = NewObject<UStaticMeshComponent>(BuildSphereActor, TEXT("BuildSphere"));
	BuildSphere->RegisterComponent();
	BuildSphereActor->SetRootComponent(BuildSphere);
	
	// Set up sphere mesh (using engine sphere)
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh)
	{
		BuildSphere->SetStaticMesh(SphereMesh);
		
		// Scale to match build radius (sphere mesh is 100 units diameter by default)
		float SphereScale = (BuildRadius * 2.0f) / 100.0f;
		BuildSphere->SetWorldScale3D(FVector(SphereScale));
		
		// Apply material if set
		if (BuildSphereMaterial)
		{
			BuildSphere->SetMaterial(0, BuildSphereMaterial);
		}
		else
		{
			// Create a simple translucent material if none provided
			UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(
				LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultTranslucentMaterial.DefaultTranslucentMaterial")), 
				BuildSphereActor);
			
			if (DynMaterial)
			{
				DynMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 0.5f, 1.0f, 0.3f)); // Blue translucent
				BuildSphere->SetMaterial(0, DynMaterial);
			}
		}
		
		// Set collision to no collision (visual only)
		BuildSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		// UE_LOG(LogTemp, Warning, TEXT("Created build sphere at %s with radius %.0f"), 
		//	*CurrentBuildLocation.ToString(), BuildRadius);
	}
}

void UBuilderComponent::DestroyBuildSphere()
{
	if (BuildSphereActor && IsValid(BuildSphereActor))
	{
		BuildSphereActor->Destroy();
		BuildSphereActor = nullptr;
		BuildSphere = nullptr;
		
		// UE_LOG(LogTemp, Warning, TEXT("Destroyed build sphere"));
	}
}