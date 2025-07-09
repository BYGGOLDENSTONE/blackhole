#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStateManager.generated.h"

UENUM(BlueprintType)
enum class EGameState : uint8
{
	None,
	MainMenu,
	Playing,
	Paused,
	GameOver,
	Transitioning
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameStateChanged, EGameState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameReset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameQuit);

/**
 * Manages overall game state transitions and ensures proper cleanup
 */
UCLASS()
class BLACKHOLE_API UGameStateManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Subsystem overrides
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// State management
	UFUNCTION(BlueprintCallable, Category = "Game State")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void PauseGame();

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void ResumeGame();

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void EndGame(bool bPlayerDied = false);

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void ResetGame();

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void RestartGame();

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void QuitGame();

	UFUNCTION(BlueprintPure, Category = "Game State")
	EGameState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Game State")
	bool IsPlaying() const { return CurrentState == EGameState::Playing; }

	UFUNCTION(BlueprintPure, Category = "Game State")
	bool IsGameOver() const { return CurrentState == EGameState::GameOver; }

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Game State")
	FOnGameStateChanged OnGameStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game State")
	FOnGameReset OnGameReset;

	UPROPERTY(BlueprintAssignable, Category = "Game State")
	FOnGameQuit OnGameQuit;

private:
	UPROPERTY()
	EGameState CurrentState;

	UPROPERTY()
	EGameState PreviousState;

	// Transition helpers
	void SetGameState(EGameState NewState);
	void PerformStateTransition(EGameState FromState, EGameState ToState);
	
	// Cleanup helpers
	void CleanupGameSystems();
	void CleanupTimers();
	void CleanupDelegates();
	void ResetPlayerState();
};