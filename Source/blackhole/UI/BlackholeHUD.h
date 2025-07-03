#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlackholeHUD.generated.h"

class ABlackholePlayerCharacter;
class UIntegrityComponent;
class UStaminaComponent;
class UWillPowerComponent;
class USlashAbilityComponent;
class USystemFreezeAbilityComponent;
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

private:
	AActor* GetTargetedActor() const;
};