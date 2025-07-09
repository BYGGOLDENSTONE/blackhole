#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "blackhole.h"
#include "Systems/GameStateManager.h"
#include "BlackholeHUD.generated.h"

class ABlackholePlayerCharacter;
class UIntegrityComponent;
class UStaminaComponent;
class UWillPowerComponent;
class UHeatComponent;
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
class UMoltenMaceSlashAbility;
class UHeatShieldAbility;
class UBlastChargeAbility;
class UHammerStrikeAbility;
class UForgeDashAbility;
class UForgeJumpAbility;
class UComboComponent;
class USimplePauseMenu;

UCLASS()
class BLACKHOLE_API ABlackholeHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABlackholeHUD();

	virtual void DrawHUD() override;
	
	// Called when the menu toggle key is pressed
	void OnMenuTogglePressed();

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
	
	// Forge abilities
	UPROPERTY()
	class UMoltenMaceSlashAbility* CachedMoltenMace;
	
	UPROPERTY()
	class UHeatShieldAbility* CachedHeatShield;
	
	UPROPERTY()
	class UBlastChargeAbility* CachedBlastCharge;
	
	UPROPERTY()
	class UHammerStrikeAbility* CachedHammerStrike;
	
	UPROPERTY()
	class UForgeDashAbility* CachedForgeDash;
	
	UPROPERTY()
	class UForgeJumpAbility* CachedForgeJump;
	
	// Combo component
	UPROPERTY()
	class UComboComponent* CachedComboComponent;

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

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FColor IntegrityColor;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FColor StaminaColor;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FColor WillPowerColor;
	
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FColor HeatColor;
	
	// Resource values cached from delegates
	float CachedWP;
	float CachedMaxWP;
	float CachedHeat;
	float CachedMaxHeat;
	
	// Ultimate mode state
	bool bUltimateModeActive;
	
	// Update methods for resource delegates
	UFUNCTION()
	void UpdateWPBar(float NewValue, float MaxValue);
	
	UFUNCTION()
	void UpdateHeatBar(float NewValue, float MaxValue);
	
	// Ultimate mode delegate
	UFUNCTION()
	void OnUltimateModeChanged(bool bActive);

private:
	AActor* GetTargetedActor() const;
	
	// Draw all abilities with their inputs and cooldowns
	void DrawAbilityInfo();
	
	// Draw debug status panel
	void DrawDebugStatus();
	
	// Draw combo status
	void DrawComboStatus();
	
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
};