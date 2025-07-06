#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "blackhole.h"
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

UCLASS()
class BLACKHOLE_API ABlackholeHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABlackholeHUD();

	virtual void DrawHUD() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	ABlackholePlayerCharacter* PlayerCharacter;
	
	UPROPERTY()
	UResourceManager* ResourceManager;

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
	
	// Update methods for resource delegates
	UFUNCTION()
	void UpdateWPBar(float NewValue, float MaxValue);
	
	UFUNCTION()
	void UpdateHeatBar(float NewValue, float MaxValue);

private:
	AActor* GetTargetedActor() const;
	
	// Draw all abilities with their inputs and cooldowns
	void DrawAbilityInfo();
	
	// Structure to hold ability display info
	struct FAbilityDisplayInfo
	{
		FString Name;
		FString Input;
		class UAbilityComponent* Ability;
		bool bIsActive;
	};
	
	// Get current path abilities
	TArray<FAbilityDisplayInfo> GetCurrentAbilities() const;
};