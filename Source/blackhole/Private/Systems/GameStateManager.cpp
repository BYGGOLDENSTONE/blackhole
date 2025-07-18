#include "Systems/GameStateManager.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Abilities/AbilityComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Debug/CrashLogger.h"

void UGameStateManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	CurrentState = EGameState::None;
	PreviousState = EGameState::None;
	
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Initialized"));
	
	// Start with main menu
	// Note: This will be called after level load, so we check the level name
	if (UWorld* World = GetWorld())
	{
		FString MapName = World->GetMapName();
		// Remove PIE prefix if in editor
		MapName.RemoveFromStart("UEDPIE_0_");
		
		if (MapName.Contains("MainMenu"))
		{
			SetGameState(EGameState::MainMenu);
		}
		else
		{
			// If we're in a game level, start playing directly
			SetGameState(EGameState::Playing);
		}
	}
}

void UGameStateManager::Deinitialize()
{
	// Clean up all systems before deinitializing
	CleanupGameSystems();
	
	// Clear all delegates
	OnGameStateChanged.Clear();
	OnGameReset.Clear();
	OnGameQuit.Clear();
	
	Super::Deinitialize();
}

void UGameStateManager::StartGame()
{
	if (CurrentState == EGameState::Playing)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameStateManager: Already playing"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Starting game"));
	
	// Reset systems before starting
	ResetPlayerState();
	
	// Resource manager is already initialized as a GameInstanceSubsystem
	// Just ensure it's ready
	if (UResourceManager* ResourceMgr = GetGameInstance()->GetSubsystem<UResourceManager>())
	{
		// ResourceManager initializes itself, we don't need to call Initialize
		UE_LOG(LogTemp, Log, TEXT("GameStateManager: ResourceManager ready"));
	}
	
	// Start threshold manager
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			ThresholdMgr->StartCombat();
		}
	}
	
	SetGameState(EGameState::Playing);
}

void UGameStateManager::PauseGame()
{
	if (CurrentState != EGameState::Playing)
	{
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Pausing game"));
	
	// The pause menu will handle actual pausing
	SetGameState(EGameState::Paused);
}

void UGameStateManager::ResumeGame()
{
	if (CurrentState != EGameState::Paused)
	{
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Resuming game"));
	
	// The pause menu will handle actual unpausing
	SetGameState(EGameState::Playing);
}

void UGameStateManager::EndGame(bool bPlayerDied)
{
	if (CurrentState == EGameState::GameOver)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("GameStateManager: Ending game (PlayerDied: %s)"), 
		bPlayerDied ? TEXT("true") : TEXT("false"));
	
	// Clean up game systems
	CleanupGameSystems();
	
	// End combat in threshold manager
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			ThresholdMgr->EndCombat();
		}
	}
	
	SetGameState(EGameState::GameOver);
}

void UGameStateManager::ResetGame()
{
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Resetting game"));
	
	// Set transitioning state to prevent issues during reset
	SetGameState(EGameState::Transitioning);
	
	// Clean up everything
	CleanupGameSystems();
	CleanupTimers();
	CleanupDelegates();
	
	// Reset all subsystems
	if (UResourceManager* ResourceMgr = GetGameInstance()->GetSubsystem<UResourceManager>())
	{
		ResourceMgr->ResetResources();
	}
	
	// Reset player
	ResetPlayerState();
	
	// Broadcast reset event
	OnGameReset.Broadcast();
	
	// For restart, we want to go directly to Playing state, not MainMenu
	// SetGameState(EGameState::MainMenu);  // Removed - don't go to main menu
	
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Game reset complete"));
}

void UGameStateManager::QuitGame()
{
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Quitting game"));
	
	// Clean up everything before quitting
	CleanupGameSystems();
	CleanupTimers();
	CleanupDelegates();
	
	// Broadcast quit event
	OnGameQuit.Broadcast();
	
	// Actually quit the game
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->ConsoleCommand("quit");
		}
	}
}

void UGameStateManager::SetGameState(EGameState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}
	
	PreviousState = CurrentState;
	CurrentState = NewState;
	
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: State changed from %s to %s"),
		*UEnum::GetValueAsString(PreviousState),
		*UEnum::GetValueAsString(CurrentState));
	
	// Perform transition logic
	PerformStateTransition(PreviousState, CurrentState);
	
	// Broadcast state change
	OnGameStateChanged.Broadcast(CurrentState);
}

void UGameStateManager::PerformStateTransition(EGameState FromState, EGameState ToState)
{
	// Handle specific transitions
	if (ToState == EGameState::GameOver)
	{
		// Debug message removed - game over
	}
	else if (ToState == EGameState::Playing && FromState == EGameState::MainMenu)
	{
		// Clear any previous messages
		if (GEngine)
		{
			GEngine->ClearOnScreenDebugMessages();
		}
	}
}

void UGameStateManager::CleanupGameSystems()
{
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Cleaning up game systems"));
	
	// Clean up timers first
	CleanupTimers();
	
	// Clean up threshold manager
	if (UWorld* World = GetWorld())
	{
		if (UThresholdManager* ThresholdMgr = World->GetSubsystem<UThresholdManager>())
		{
			ThresholdMgr->EndCombat();
		}
	}
	
	// Clean up any active abilities on player
	if (UWorld* World = GetWorld())
	{
		if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(
			UGameplayStatics::GetPlayerCharacter(World, 0)))
		{
			// Get all ability components and disable them
			TArray<UActorComponent*> AbilityComponents = Player->GetComponents().Array();
			for (UActorComponent* Component : AbilityComponents)
			{
				if (Component && Component->IsA<UAbilityComponent>())
				{
					Component->SetComponentTickEnabled(false);
				}
			}
		}
	}
}

void UGameStateManager::CleanupTimers()
{
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Cleaning up all timers"));
	
	// Clear all timers in the world
	if (UWorld* World = GetWorld())
	{
		// Clear timers for this object
		World->GetTimerManager().ClearAllTimersForObject(this);
		
		// Clear timers for player if it exists
		if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(
			UGameplayStatics::GetPlayerCharacter(World, 0)))
		{
			World->GetTimerManager().ClearAllTimersForObject(Player);
		}
	}
}

void UGameStateManager::CleanupDelegates()
{
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Cleaning up delegates"));
	
	// This is handled by individual systems in their EndPlay/Deinitialize
	// But we ensure our own delegates are clean
	OnGameStateChanged.Clear();
	OnGameReset.Clear();
	OnGameQuit.Clear();
}

void UGameStateManager::RestartGame()
{
	CRASH_CHECKPOINT_MSG("RestartGame Start");
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Restarting game"));
	
	// Set transitioning state
	CRASH_CHECKPOINT_MSG("Setting transitioning state");
	SetGameState(EGameState::Transitioning);
	
	CRASH_CHECKPOINT_MSG("Getting World");
	if (UWorld* World = GetWorld())
	{
		// Clean up game systems
		CRASH_CHECKPOINT_MSG("Cleaning up game systems");
		CleanupGameSystems();
		
		// Reset resource manager
		CRASH_CHECKPOINT_MSG("Resetting resource manager");
		if (UResourceManager* ResourceMgr = GetGameInstance()->GetSubsystem<UResourceManager>())
		{
			ResourceMgr->ResetResources();
		}
		
		// Get current level name
		CRASH_CHECKPOINT_MSG("Getting current level name");
		FString CurrentLevelName = World->GetMapName();
		// Remove PIE prefix if in editor
		CurrentLevelName.RemoveFromStart("UEDPIE_0_");
		
		UE_LOG(LogTemp, Log, TEXT("GameStateManager: Reloading level %s"), *CurrentLevelName);
		
		// Use ServerTravel for more reliable level reload
		CRASH_CHECKPOINT_MSG("Getting PlayerController");
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			// Ensure we're not paused
			CRASH_CHECKPOINT_MSG("Unpausing");
			UGameplayStatics::SetGamePaused(World, false);
			
			// Clean up input state
			CRASH_CHECKPOINT_MSG("Setting input mode");
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
			
			// Use ClientTravel for single player
			CRASH_CHECKPOINT_MSG(FString::Printf(TEXT("Calling ClientTravel to %s"), *CurrentLevelName));
			PC->ClientTravel(CurrentLevelName, ETravelType::TRAVEL_Absolute, false);
			
			CRASH_CHECKPOINT_MSG("RestartGame End");
		}
		else
		{
			CRASH_LOG_ERROR("No PlayerController found");
		}
	}
	else
	{
		CRASH_LOG_ERROR("No World found");
	}
}

void UGameStateManager::ResetPlayerState()
{
	UE_LOG(LogTemp, Log, TEXT("GameStateManager: Resetting player state"));
	
	if (UWorld* World = GetWorld())
	{
		if (ABlackholePlayerCharacter* Player = Cast<ABlackholePlayerCharacter>(
			UGameplayStatics::GetPlayerCharacter(World, 0)))
		{
			// Reset death state
			if (Player->IsDead())
			{
				// This will be reset in BeginPlay, but we can ensure input is re-enabled
				if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
				{
					Player->EnableInput(PC);
				}
			}
			
			// Reset movement
			if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
			{
				Movement->SetMovementMode(MOVE_Walking);
				Movement->MaxWalkSpeed = 600.0f; // Default walk speed
			}
			
			// Reset mesh physics
			if (USkeletalMeshComponent* Mesh = Player->GetMesh())
			{
				Mesh->SetSimulatePhysics(false);
				Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}
		}
	}
}