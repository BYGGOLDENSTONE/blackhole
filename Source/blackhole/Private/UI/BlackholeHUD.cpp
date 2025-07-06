#include "UI/BlackholeHUD.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Attributes/StaminaComponent.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Components/Attributes/HeatComponent.h"
#include "Systems/ResourceManager.h"
#include "Components/Abilities/Player/Basic/SlashAbilityComponent.h"
// #include "Components/Abilities/Player/SystemFreezeAbilityComponent.h" // Removed
#include "Components/Abilities/Player/Basic/KillAbilityComponent.h"
#include "Components/Abilities/Player/Hacker/GravityPullAbilityComponent.h"
#include "Components/Abilities/Player/Utility/HackerDashAbility.h"
#include "Components/Abilities/Player/Utility/ForgeDashAbility.h"
#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "Components/Abilities/Player/Utility/ForgeJumpAbility.h"
#include "Components/Abilities/Player/Hacker/PulseHackAbility.h"
#include "Components/Abilities/Player/Hacker/FirewallBreachAbility.h"
#include "Components/Abilities/Player/Forge/MoltenMaceSlashAbility.h"
#include "Components/Abilities/Player/Forge/HeatShieldAbility.h"
#include "Components/Abilities/Player/Forge/BlastChargeAbility.h"
#include "Components/Abilities/Player/Forge/HammerStrikeAbility.h"
#include "Components/Abilities/AbilityComponent.h"
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

	// Draw WP only in Hacker path
	int resourceBarIndex = 2;
	if (PlayerCharacter->GetCurrentPath() == ECharacterPath::Hacker)
	{
		DrawAttribute("Will Power", CachedWP, CachedMaxWP, 
					  StartX, StartY + VerticalSpacing * resourceBarIndex, WillPowerColor);
		resourceBarIndex++;
	}
	
	// Draw Heat bar only in Forge path
	if (ResourceManager && ResourceManager->IsHeatSystemActive())
	{
		DrawAttribute("Heat", CachedHeat, CachedMaxHeat,
					  StartX, StartY + VerticalSpacing * resourceBarIndex, HeatColor);
		resourceBarIndex++;
	}
	
	// Draw current path
	if (PlayerCharacter)
	{
		FString PathText = FString::Printf(TEXT("Path: %s"), *PlayerCharacter->GetCurrentPathName());
		DrawText(PathText, 
				 PlayerCharacter->GetCurrentPath() == ECharacterPath::Hacker ? FColor::Cyan : FColor::Orange, 
				 StartX, StartY + VerticalSpacing * resourceBarIndex);
	}

	// Draw all abilities
	DrawAbilityInfo();

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

TArray<ABlackholeHUD::FAbilityDisplayInfo> ABlackholeHUD::GetCurrentAbilities() const
{
	TArray<FAbilityDisplayInfo> Abilities;
	
	if (!PlayerCharacter)
	{
		return Abilities;
	}
	
	bool bIsHacker = PlayerCharacter->GetCurrentPath() == ECharacterPath::Hacker;
	
	// Always available abilities
	if (auto* Slash = PlayerCharacter->FindComponentByClass<USlashAbilityComponent>())
	{
		Abilities.Add({TEXT("Slash"), TEXT("LMB"), Slash, true});
	}
	
	if (auto* Kill = PlayerCharacter->FindComponentByClass<UKillAbilityComponent>())
	{
		Abilities.Add({TEXT("Kill (Debug)"), TEXT("K"), Kill, true});
	}
	
	// Path-specific abilities
	if (bIsHacker)
	{
		// Hacker abilities
		if (auto* FirewallBreach = PlayerCharacter->FindComponentByClass<UFirewallBreachAbility>())
		{
			Abilities.Add({TEXT("Firewall Breach"), TEXT("RMB"), FirewallBreach, true});
		}
		
		if (auto* PulseHack = PlayerCharacter->FindComponentByClass<UPulseHackAbility>())
		{
			Abilities.Add({TEXT("Pulse Hack"), TEXT("Q"), PulseHack, true});
		}
		
		if (auto* GravityPull = PlayerCharacter->FindComponentByClass<UGravityPullAbilityComponent>())
		{
			Abilities.Add({TEXT("Gravity Pull"), TEXT("E"), GravityPull, true});
		}
		
		if (auto* HackerDash = PlayerCharacter->FindComponentByClass<UHackerDashAbility>())
		{
			Abilities.Add({TEXT("Hacker Dash"), TEXT("Shift"), HackerDash, true});
		}
		
		if (auto* HackerJump = PlayerCharacter->FindComponentByClass<UHackerJumpAbility>())
		{
			Abilities.Add({TEXT("Hacker Jump"), TEXT("Space"), HackerJump, true});
		}
		
		// R slot reserved for Hacker
		Abilities.Add({TEXT("(Reserved)"), TEXT("R"), nullptr, false});
	}
	else
	{
		// Forge abilities
		if (auto* MoltenMace = PlayerCharacter->FindComponentByClass<UMoltenMaceSlashAbility>())
		{
			Abilities.Add({TEXT("Molten Mace"), TEXT("RMB"), MoltenMace, true});
		}
		
		if (auto* HeatShield = PlayerCharacter->FindComponentByClass<UHeatShieldAbility>())
		{
			Abilities.Add({TEXT("Heat Shield"), TEXT("Q"), HeatShield, true});
		}
		
		if (auto* BlastCharge = PlayerCharacter->FindComponentByClass<UBlastChargeAbility>())
		{
			Abilities.Add({TEXT("Blast Charge"), TEXT("E"), BlastCharge, true});
		}
		
		if (auto* HammerStrike = PlayerCharacter->FindComponentByClass<UHammerStrikeAbility>())
		{
			Abilities.Add({TEXT("Hammer Strike"), TEXT("R"), HammerStrike, true});
		}
		
		if (auto* ForgeDash = PlayerCharacter->FindComponentByClass<UForgeDashAbility>())
		{
			Abilities.Add({TEXT("Forge Dash"), TEXT("Shift"), ForgeDash, true});
		}
		
		if (auto* ForgeJump = PlayerCharacter->FindComponentByClass<UForgeJumpAbility>())
		{
			Abilities.Add({TEXT("Forge Jump"), TEXT("Space"), ForgeJump, true});
		}
	}
	
	// F slot reserved for both paths
	Abilities.Add({TEXT("(Ultimate)"), TEXT("F"), nullptr, false});
	
	return Abilities;
}

void ABlackholeHUD::DrawAbilityInfo()
{
	if (!Canvas)
	{
		return;
	}
	
	TArray<FAbilityDisplayInfo> Abilities = GetCurrentAbilities();
	
	// Draw abilities in a grid at the bottom of the screen
	float StartX = 50.0f;
	float StartY = Canvas->SizeY - 200.0f;
	float BoxWidth = 120.0f;
	float BoxHeight = 80.0f;
	float Spacing = 10.0f;
	
	// Also draw a debug info panel on the right side
	float DebugX = Canvas->SizeX - 300.0f;
	float DebugY = 200.0f;
	
	DrawText(TEXT("=== ABILITY DEBUG INFO ==="), FColor::Yellow, DebugX, DebugY);
	DebugY += 20.0f;
	
	for (int32 i = 0; i < Abilities.Num(); i++)
	{
		const FAbilityDisplayInfo& AbilityInfo = Abilities[i];
		
		float X = StartX + (i % 6) * (BoxWidth + Spacing);
		float Y = StartY + (i / 6) * (BoxHeight + Spacing);
		
		// Draw ability box
		FColor BoxColor = AbilityInfo.bIsActive ? FColor(32, 32, 32) : FColor(16, 16, 16);
		DrawRect(FColor::Black, X - 2, Y - 2, BoxWidth + 4, BoxHeight + 4);
		DrawRect(BoxColor, X, Y, BoxWidth, BoxHeight);
		
		// Draw ability name
		DrawText(AbilityInfo.Name, FColor::White, X + 5, Y + 5);
		
		// Draw input key
		FColor InputColor = AbilityInfo.bIsActive ? FColor::Green : FColor(128, 128, 128);
		DrawText(FString::Printf(TEXT("[%s]"), *AbilityInfo.Input), InputColor, X + 5, Y + 25);
		
		// Draw cooldown and resource cost if ability is active
		if (AbilityInfo.bIsActive && AbilityInfo.Ability)
		{
			float CooldownPercent = AbilityInfo.Ability->GetCooldownPercentage();
			
			// Draw cooldown bar
			if (CooldownPercent > 0.0f)
			{
				float CooldownWidth = BoxWidth - 10;
				float CooldownHeight = 5.0f;
				DrawRect(FColor(64, 64, 64), X + 5, Y + 50, CooldownWidth, CooldownHeight);
				DrawRect(FColor::Red, X + 5, Y + 50, CooldownWidth * CooldownPercent, CooldownHeight);
				
				// Draw cooldown time
				float TimeRemaining = AbilityInfo.Ability->GetCooldownRemaining();
				DrawText(FString::Printf(TEXT("%.1fs"), TimeRemaining), FColor::Red, X + 5, Y + 60);
			}
			else
			{
				DrawText(TEXT("Ready"), FColor::Green, X + 5, Y + 50);
			}
			
			// Draw debug info on the right panel
			FString DebugInfo = FString::Printf(TEXT("%s: CD %.1fs (%.0f%%)"), 
				*AbilityInfo.Name, 
				AbilityInfo.Ability->GetCooldownRemaining(),
				CooldownPercent * 100.0f);
			DrawText(DebugInfo, FColor::White, DebugX, DebugY);
			DebugY += 18.0f;
			
			// Draw resource costs
			float StaminaCost = AbilityInfo.Ability->GetCost();
			float WPCost = AbilityInfo.Ability->GetWPCost();
			float HeatCost = AbilityInfo.Ability->GetHeatCost();
			
			FString CostText;
			if (StaminaCost > 0)
			{
				CostText += FString::Printf(TEXT("Stamina: %.0f "), StaminaCost);
			}
			if (WPCost > 0)
			{
				CostText += FString::Printf(TEXT("WP: +%.0f "), WPCost);
			}
			if (HeatCost > 0)
			{
				CostText += FString::Printf(TEXT("Heat: +%.0f"), HeatCost);
			}
			
			if (!CostText.IsEmpty())
			{
				DrawText(CostText, FColor(200, 200, 200), DebugX + 20, DebugY);
				DebugY += 18.0f;
			}
		}
	}
}