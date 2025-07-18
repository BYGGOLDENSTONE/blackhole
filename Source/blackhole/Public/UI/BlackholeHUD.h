#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "blackhole.h"
#include "Systems/GameStateManager.h"
#include "BlackholeHUD.generated.h"

class ABlackholePlayerCharacter;
class UWillPowerComponent;
class UResourceManager;
class USlashAbilityComponent;
// class USystemFreezeAbilityComponent; // Removed
class UKillAbilityComponent;
class UFirewallBreachAbility;
class UPulseHackAbility;
class UGravityPullAbilityComponent;
class UDataSpikeAbility;
class USystemOverrideAbility;
class UHackerDashAbility;
class UHackerJumpAbility;
class USimplePauseMenu;
class UThresholdManager;
class UWallRunComponent;

UCLASS()
class BLACKHOLE_API ABlackholeHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABlackholeHUD();

	virtual void DrawHUD() override;
	
	// Called when the menu toggle key is pressed
	void OnMenuTogglePressed();
	
	// Notification System - Public methods for external access
	UFUNCTION()
	void OnPsiDisruptorBuildStarted(const FVector& BuildLocation);
	
	UFUNCTION()
	void OnPsiDisruptorBuildComplete();
	
	UFUNCTION()
	void OnMindmeldStarted(float CastTime, AActor* Caster = nullptr);
	
	UFUNCTION()
	void OnMindmeldComplete();
	
	UFUNCTION()
	void OnMindmeldInterrupted();
	
	// General notification system
	void AddNotification(const FString& Message, const FColor& Color, float Duration = 5.0f);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Menu management
	void ShowMainMenu();
	void ShowPauseMenu();
	void ShowGameOverMenu();
	void HideAllMenus();
	
	// Input handling
	void SetupInputComponent();
	void OnEscapePressed();
	
	// State change handler
	UFUNCTION()
	void OnGameStateChanged(EGameState NewState);

	UPROPERTY()
	ABlackholePlayerCharacter* PlayerCharacter;
	
	// Menu widget classes - must be set in Blueprint
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UMainMenuWidget> MainMenuWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UPauseMenuWidget> PauseMenuWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UGameOverWidget> GameOverWidgetClass;
	
	// Menu widget instances
	UPROPERTY()
	class UMainMenuWidget* MainMenuWidget;
	
	UPROPERTY()
	class UPauseMenuWidget* PauseMenuWidget;
	
	UPROPERTY()
	class USimplePauseMenu* SimplePauseMenu;
	
	UPROPERTY()
	class UGameOverWidget* GameOverWidget;
	
	UPROPERTY()
	class UGameStateManager* GameStateManager;
	
	UPROPERTY()
	UResourceManager* ResourceManager;
	
	UPROPERTY()
	class UThresholdManager* ThresholdManager;
	
	// Cached ability components
	UPROPERTY()
	class USlashAbilityComponent* CachedSlashAbility;
	
	UPROPERTY()
	class UKillAbilityComponent* CachedKillAbility;
	
	// Hacker abilities
	UPROPERTY()
	class UFirewallBreachAbility* CachedFirewallBreach;
	
	UPROPERTY()
	class UPulseHackAbility* CachedPulseHack;
	
	UPROPERTY()
	class UGravityPullAbilityComponent* CachedGravityPull;
	
	UPROPERTY()
	class UDataSpikeAbility* CachedDataSpike;
	
	UPROPERTY()
	class USystemOverrideAbility* CachedSystemOverride;
	
	UPROPERTY()
	class UHackerDashAbility* CachedHackerDash;
	
	UPROPERTY()
	class UHackerJumpAbility* CachedHackerJump;
	

	void DrawAttribute(const FString& Name, float Current, float Max, float X, float Y, const FColor& Color);
	void DrawAbilityCooldown(const FString& Name, float CooldownPercent, float X, float Y);
	void DrawCrosshair();
	void DrawTargetInfo();

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float AttributeBarWidth;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float AttributeBarHeight;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float CooldownIconSize;
	
	// Performance optimization
	UPROPERTY(EditDefaultsOnly, Category = "Performance")
	float CacheUpdateInterval = 0.1f; // Update cache every 0.1 seconds
	
	// Cached values for performance
	float LastCacheUpdateTime = 0.0f;
	float CachedWPPercent = 0.0f;
	float CachedCurrentWP = 0.0f;
	float CachedMaxWP = 0.0f;
	bool bIsUltimateMode = false;
	TArray<float> CachedCooldownPercents;
	FString CachedDebugInfo;
	
	// Velocity/Speed cached values
	float CachedVelocity = 0.0f;
	float CachedSpeed = 0.0f;
	FVector CachedVelocityVector = FVector::ZeroVector;
	bool bIsWallRunning = false;
	bool bIsDashing = false;
	
	// Update cached values
	void UpdateCachedValues();
	
	// Draw critical timer display
	void DrawCriticalTimer();
	
	// Draw wall run timer display
	void DrawWallRunTimer();
	
	// Draw velocity/speed indicator
	void DrawVelocityIndicator();

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FColor WillPowerColor;
	
	// Resource values cached from delegates
	float CachedWP;
	
	// Ultimate mode state
	bool bUltimateModeActive;
	
	// Update methods for resource delegates
	UFUNCTION()
	void UpdateWPBar(float NewValue, float MaxValue);
	
	// Ultimate mode delegate
	UFUNCTION()
	void OnUltimateModeChanged(bool bActive);
	
	// Critical timer variables
	bool bCriticalTimerActive = false;
	float CriticalTimeRemaining = 0.0f;
	
	// Critical timer delegates
	UFUNCTION()
	void OnCriticalTimerUpdate(float TimeRemaining);
	
	UFUNCTION()
	void OnCriticalTimerExpired();
	

private:
	AActor* GetTargetedActor() const;
	
	// Draw all abilities with their inputs and cooldowns
	void DrawAbilityInfo();
	
	// Draw debug status panel
	void DrawDebugStatus();
	
	// Structure to hold ability display info
	struct FAbilityDisplayInfo
	{
		FString Name;
		FString Input;
		class UAbilityComponent* Ability;
		bool bIsActive;
		bool bIsDisabled;
		bool bIsInUltimateMode;
		bool bIsBasicAbility;
	};
	
	// Get current path abilities
	TArray<FAbilityDisplayInfo> GetCurrentAbilities() const;
	
	// String formatting helpers to avoid per-frame allocations
	TCHAR AttributeTextBuffer[64];
	TCHAR TargetTextBuffer[128];
	TCHAR CooldownTextBuffer[32];
	TCHAR DebugTextBuffer[256];
	
	// Notification system
	struct FNotification
	{
		FString Message;
		FColor Color;
		float TimeRemaining;
		float TotalDuration;
	};
	
	TArray<FNotification> ActiveNotifications;
	
	// Progress tracking for enemy abilities
	float PsiDisruptorBuildProgress = 0.0f;
	float PsiDisruptorBuildTime = 20.0f;
	bool bShowingPsiDisruptorProgress = false;
	FVector PsiDisruptorLocation;
	
	float MindmeldProgress = 0.0f;
	float MindmeldCastTime = 30.0f;
	bool bShowingMindmeldProgress = false;
	AActor* MindmeldCaster = nullptr;
	
	// Helper methods
	void UpdateNotifications(float DeltaTime);
	void DrawNotifications();
	void DrawEnemyAbilityProgress();
};