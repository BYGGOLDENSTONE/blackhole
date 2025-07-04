#include "UI/BlackholeHUD.h"
#include "../Player/BlackholePlayerCharacter.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Attributes/StaminaComponent.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Components/Attributes/HeatComponent.h"
#include "Systems/ResourceManager.h"
#include "Components/Abilities/Player/Basic/SlashAbilityComponent.h"
// #include "Components/Abilities/Player/SystemFreezeAbilityComponent.h" // Removed
#include "Components/Abilities/Player/Basic/KillAbilityComponent.h"
#include "Components/Abilities/Player/Hacker/GravityPullAbilityComponent.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ABlackholeHUD::ABlackholeHUD()
{
	AttributeBarWidth = 200.0f;
	AttributeBarHeight = 20.0f;
	CooldownIconSize = 50.0f;

	IntegrityColor = FColor::Red;
	StaminaColor = FColor::Green;
	WillPowerColor = FColor::Blue;
	HeatColor = FColor::Orange;
	
	// Initialize cached values
	CachedWP = 100.0f;
	CachedMaxWP = 100.0f;
	CachedHeat = 0.0f;
	CachedMaxHeat = 100.0f;
}

void ABlackholeHUD::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	// Bind to ResourceManager events
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		ResourceManager = GameInstance->GetSubsystem<UResourceManager>();
		if (ResourceManager)
		{
			ResourceManager->OnWillPowerChanged.AddDynamic(this, &ABlackholeHUD::UpdateWPBar);
			ResourceManager->OnHeatChanged.AddDynamic(this, &ABlackholeHUD::UpdateHeatBar);
			
			// Initialize with current values
			CachedWP = ResourceManager->GetCurrentWillPower();
			CachedMaxWP = ResourceManager->GetMaxWillPower();
			CachedHeat = ResourceManager->GetCurrentHeat();
			CachedMaxHeat = ResourceManager->GetMaxHeat();
		}
	}
}

void ABlackholeHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!PlayerCharacter || !Canvas)
	{
		return;
	}

	DrawCrosshair();

	float StartX = 50.0f;
	float StartY = 50.0f;
	float VerticalSpacing = 30.0f;

	if (UIntegrityComponent* Integrity = PlayerCharacter->FindComponentByClass<UIntegrityComponent>())
	{
		DrawAttribute("Integrity", Integrity->GetCurrentValue(), Integrity->GetMaxValue(), 
					  StartX, StartY, IntegrityColor);
	}

	if (UStaminaComponent* Stamina = PlayerCharacter->FindComponentByClass<UStaminaComponent>())
	{
		DrawAttribute("Stamina", Stamina->GetCurrentValue(), Stamina->GetMaxValue(), 
					  StartX, StartY + VerticalSpacing, StaminaColor);
	}

	// Draw WP from cached values (updated via delegates)
	DrawAttribute("Will Power", CachedWP, CachedMaxWP, 
				  StartX, StartY + VerticalSpacing * 2, WillPowerColor);
	
	// Draw Heat bar only in Forge path
	if (ResourceManager && ResourceManager->IsHeatSystemActive())
	{
		DrawAttribute("Heat", CachedHeat, CachedMaxHeat,
					  StartX, StartY + VerticalSpacing * 3, HeatColor);
	}
	
	// Draw current path
	if (PlayerCharacter)
	{
		FString PathText = FString::Printf(TEXT("Path: %s"), *PlayerCharacter->GetCurrentPathName());
		DrawText(PathText, 
				 PlayerCharacter->GetCurrentPath() == ECharacterPath::Hacker ? FColor::Cyan : FColor::Orange, 
				 StartX, StartY + VerticalSpacing * 4);
	}

	float CooldownStartX = Canvas->SizeX * 0.5f - (CooldownIconSize * 1.5f);
	float CooldownY = Canvas->SizeY - 100.0f;

	if (USlashAbilityComponent* Slash = PlayerCharacter->FindComponentByClass<USlashAbilityComponent>())
	{
		DrawAbilityCooldown("LMB", Slash->GetCooldownPercentage(), CooldownStartX, CooldownY);
	}

	// SystemFreeze removed

	if (UKillAbilityComponent* Kill = PlayerCharacter->FindComponentByClass<UKillAbilityComponent>())
	{
		DrawAbilityCooldown("K", Kill->GetCooldownPercentage(), 
							CooldownStartX + CooldownIconSize + 10, CooldownY);
	}

	if (UGravityPullAbilityComponent* GravityPull = PlayerCharacter->FindComponentByClass<UGravityPullAbilityComponent>())
	{
		DrawAbilityCooldown("G", GravityPull->GetCooldownPercentage(), 
							CooldownStartX + (CooldownIconSize + 10) * 2, CooldownY);
	}

	DrawTargetInfo();
}

void ABlackholeHUD::DrawAttribute(const FString& Name, float Current, float Max, float X, float Y, const FColor& Color)
{
	DrawText(Name, Color, X, Y);

	float BarX = X + 100.0f;
	float BarY = Y;
	float FilledWidth = (Current / Max) * AttributeBarWidth;

	DrawRect(FColor::Black, BarX - 2, BarY - 2, AttributeBarWidth + 4, AttributeBarHeight + 4);
	DrawRect(FColor(64, 64, 64), BarX, BarY, AttributeBarWidth, AttributeBarHeight);
	DrawRect(Color, BarX, BarY, FilledWidth, AttributeBarHeight);

	FString ValueText = FString::Printf(TEXT("%.0f/%.0f"), Current, Max);
	DrawText(ValueText, FColor::White, BarX + AttributeBarWidth + 10, Y);
}

void ABlackholeHUD::DrawAbilityCooldown(const FString& Name, float CooldownPercent, float X, float Y)
{
	DrawRect(FColor::Black, X - 2, Y - 2, CooldownIconSize + 4, CooldownIconSize + 4);
	
	if (CooldownPercent > 0.0f)
	{
		DrawRect(FColor(32, 32, 32), X, Y, CooldownIconSize, CooldownIconSize);
		
		float CooldownHeight = CooldownIconSize * CooldownPercent;
		DrawRect(FColor(64, 64, 64), X, Y + (CooldownIconSize - CooldownHeight), 
				 CooldownIconSize, CooldownHeight);
	}
	else
	{
		DrawRect(FColor(96, 96, 96), X, Y, CooldownIconSize, CooldownIconSize);
	}

	float TextX = X + (CooldownIconSize * 0.5f) - 5.0f;
	float TextY = Y + (CooldownIconSize * 0.5f) - 10.0f;
	DrawText(Name, FColor::White, TextX, TextY);
}

void ABlackholeHUD::DrawCrosshair()
{
	float CenterX = Canvas->SizeX * 0.5f;
	float CenterY = Canvas->SizeY * 0.5f;
	float CrosshairSize = 10.0f;

	DrawLine(CenterX - CrosshairSize, CenterY, CenterX + CrosshairSize, CenterY, FColor::White, 2.0f);
	DrawLine(CenterX, CenterY - CrosshairSize, CenterX, CenterY + CrosshairSize, FColor::White, 2.0f);
}

void ABlackholeHUD::DrawTargetInfo()
{
	AActor* Target = GetTargetedActor();
	if (!Target)
	{
		return;
	}

	float InfoX = Canvas->SizeX * 0.5f - 100.0f;
	float InfoY = 100.0f;

	DrawText(FString::Printf(TEXT("Target: %s"), *Target->GetName()), FColor::Yellow, InfoX, InfoY);

	if (UIntegrityComponent* TargetIntegrity = Target->FindComponentByClass<UIntegrityComponent>())
	{
		DrawAttribute("Enemy HP", TargetIntegrity->GetCurrentValue(), TargetIntegrity->GetMaxValue(),
					  InfoX - 100.0f, InfoY + 20.0f, FColor::Red);
	}
}

AActor* ABlackholeHUD::GetTargetedActor() const
{
	if (!PlayerCharacter)
	{
		return nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController());
	if (!PC)
	{
		return nullptr;
	}

	FVector Start = PlayerCharacter->GetActorLocation();
	FVector Forward = PC->GetControlRotation().Vector();
	FVector End = Start + (Forward * 5000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PlayerCharacter);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
	{
		return HitResult.GetActor();
	}

	return nullptr;
}

void ABlackholeHUD::UpdateWPBar(float NewValue, float MaxValue)
{
	CachedWP = NewValue;
	CachedMaxWP = MaxValue;
}

void ABlackholeHUD::UpdateHeatBar(float NewValue, float MaxValue)
{
	CachedHeat = NewValue;
	CachedMaxHeat = MaxValue;
	
	// Flash warning at 80%
	float Percent = MaxValue > 0.0f ? NewValue / MaxValue : 0.0f;
	if (Percent >= 0.8f)
	{
		// TODO: Trigger warning animation
		UE_LOG(LogTemp, Warning, TEXT("Heat warning: %.1f%%"), Percent * 100.0f);
	}
}