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
#include "Components/Abilities/UtilityAbility.h"
#include "Components/Abilities/Player/Hacker/PulseHackAbility.h"
#include "Components/Abilities/Player/Hacker/FirewallBreachAbility.h"
#include "Components/Abilities/Player/Forge/MoltenMaceSlashAbility.h"
#include "Components/Abilities/Player/Forge/HeatShieldAbility.h"
#include "Components/Abilities/Player/Forge/BlastChargeAbility.h"
#include "Components/Abilities/Player/Forge/HammerStrikeAbility.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Components/ComboComponent.h"
#include "Systems/ThresholdManager.h"
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
	bUltimateModeActive = false;
}

void ABlackholeHUD::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	// Cache all ability components
	if (PlayerCharacter)
	{
		// Basic abilities
		CachedSlashAbility = PlayerCharacter->FindComponentByClass<USlashAbilityComponent>();
		CachedKillAbility = PlayerCharacter->FindComponentByClass<UKillAbilityComponent>();
		
		// Hacker abilities
		CachedFirewallBreach = PlayerCharacter->FindComponentByClass<UFirewallBreachAbility>();
		CachedPulseHack = PlayerCharacter->FindComponentByClass<UPulseHackAbility>();
		CachedGravityPull = PlayerCharacter->FindComponentByClass<UGravityPullAbilityComponent>();
		CachedHackerDash = PlayerCharacter->FindComponentByClass<UHackerDashAbility>();
		CachedHackerJump = PlayerCharacter->FindComponentByClass<UHackerJumpAbility>();
		
		// Forge abilities
		CachedMoltenMace = PlayerCharacter->FindComponentByClass<UMoltenMaceSlashAbility>();
		CachedHeatShield = PlayerCharacter->FindComponentByClass<UHeatShieldAbility>();
		CachedBlastCharge = PlayerCharacter->FindComponentByClass<UBlastChargeAbility>();
		CachedHammerStrike = PlayerCharacter->FindComponentByClass<UHammerStrikeAbility>();
		CachedForgeDash = PlayerCharacter->FindComponentByClass<UForgeDashAbility>();
		CachedForgeJump = PlayerCharacter->FindComponentByClass<UForgeJumpAbility>();
	}
	
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
		
		// Get ThresholdManager (it's a WorldSubsystem, not GameInstanceSubsystem)
		if (UWorld* World = GetWorld())
		{
			ThresholdManager = World->GetSubsystem<UThresholdManager>();
			if (ThresholdManager)
			{
				ThresholdManager->OnUltimateModeActivated.AddDynamic(this, &ABlackholeHUD::OnUltimateModeChanged);
			}
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
	
	// Draw debug status on the right
	DrawDebugStatus();

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
	
	// Special handling for WP bar with thresholds
	if (Name == "Will Power")
	{
		// Draw the filled portion
		if (FilledWidth > 0)
		{
			// Different colors for different thresholds
			FColor WPColor = Color;
			float Percent = Current / Max;
			
			if (Percent >= 1.0f)
			{
				// Ultimate mode - pulsing red/white
				WPColor = FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) > 0 ? FColor::Red : FColor::White;
			}
			else if (Percent >= 0.5f)
			{
				// Buffed state - bright cyan
				WPColor = FColor::Cyan;
			}
			
			DrawRect(WPColor, BarX, BarY, FilledWidth, AttributeBarHeight);
		}
		
		// Draw threshold markers
		float Threshold50 = BarX + (0.5f * AttributeBarWidth);
		DrawLine(Threshold50, BarY - 2, Threshold50, BarY + AttributeBarHeight + 2, FColor::Yellow, 2.0f);
		
		// Draw threshold text
		if (Current >= Max)
		{
			DrawText(TEXT("ULTIMATE!"), FColor::Red, BarX + AttributeBarWidth / 2 - 30, BarY - 15);
		}
		else if (Current >= Max * 0.5f)
		{
			DrawText(TEXT("BUFFED"), FColor::Cyan, BarX + AttributeBarWidth / 2 - 20, BarY - 15);
		}
	}
	else
	{
		DrawRect(Color, BarX, BarY, FilledWidth, AttributeBarHeight);
	}

	// Use pre-allocated buffer to avoid per-frame allocation
	FCString::Sprintf(AttributeTextBuffer, TEXT("%.0f/%.0f"), Current, Max);
	DrawText(AttributeTextBuffer, FColor::White, BarX + AttributeBarWidth + 10, Y);
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

	// Use pre-allocated buffer to avoid per-frame allocation
	FCString::Sprintf(TargetTextBuffer, TEXT("Target: %s"), *Target->GetName());
	DrawText(TargetTextBuffer, FColor::Yellow, InfoX, InfoY);

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
	FVector End = Start + (Forward * GameplayConfig::Abilities::Defaults::RANGE * 5.0f); // 5x default range for targeting

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
	if (CachedSlashAbility)
	{
		bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedSlashAbility) : false;
		bool bUltimate = CachedSlashAbility->IsInUltimateMode();
		bool bBasic = CachedSlashAbility->IsBasicAbility();
		Abilities.Add({TEXT("Slash"), TEXT("LMB"), CachedSlashAbility, true, bDisabled, bUltimate, bBasic});
	}
	
	if (CachedKillAbility)
	{
		bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedKillAbility) : false;
		bool bUltimate = CachedKillAbility->IsInUltimateMode();
		bool bBasic = CachedKillAbility->IsBasicAbility();
		Abilities.Add({TEXT("Kill (Debug)"), TEXT("K"), CachedKillAbility, true, bDisabled, bUltimate, bBasic});
	}
	
	// Path-specific abilities
	if (bIsHacker)
	{
		// Hacker abilities
		if (CachedFirewallBreach)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedFirewallBreach) : false;
			bool bUltimate = CachedFirewallBreach->IsInUltimateMode();
			bool bBasic = CachedFirewallBreach->IsBasicAbility();
			FString Name = bUltimate ? TEXT("TOTAL SYSTEM COMPROMISE") : TEXT("Firewall Breach");
			Abilities.Add({Name, TEXT("RMB"), CachedFirewallBreach, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedPulseHack)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedPulseHack) : false;
			bool bUltimate = CachedPulseHack->IsInUltimateMode();
			bool bBasic = CachedPulseHack->IsBasicAbility();
			FString Name = bUltimate ? TEXT("SYSTEM OVERLOAD") : TEXT("Pulse Hack");
			Abilities.Add({Name, TEXT("Q"), CachedPulseHack, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedGravityPull)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedGravityPull) : false;
			bool bUltimate = CachedGravityPull->IsInUltimateMode();
			bool bBasic = CachedGravityPull->IsBasicAbility();
			FString Name = bUltimate ? TEXT("SINGULARITY") : TEXT("Gravity Pull");
			Abilities.Add({Name, TEXT("E"), CachedGravityPull, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedHackerDash)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedHackerDash) : false;
			bool bUltimate = CachedHackerDash->IsInUltimateMode();
			bool bBasic = CachedHackerDash->IsBasicAbility();
			Abilities.Add({TEXT("Hacker Dash"), TEXT("Shift"), CachedHackerDash, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedHackerJump)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedHackerJump) : false;
			bool bUltimate = CachedHackerJump->IsInUltimateMode();
			bool bBasic = CachedHackerJump->IsBasicAbility();
			Abilities.Add({TEXT("Hacker Jump"), TEXT("Space"), CachedHackerJump, true, bDisabled, bUltimate, bBasic});
		}
		
		// R slot reserved for Hacker
		Abilities.Add({TEXT("(Reserved)"), TEXT("R"), nullptr, false, false, false, false});
	}
	else
	{
		// Forge abilities
		if (CachedMoltenMace)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedMoltenMace) : false;
			bool bUltimate = CachedMoltenMace->IsInUltimateMode();
			bool bBasic = CachedMoltenMace->IsBasicAbility();
			Abilities.Add({TEXT("Molten Mace"), TEXT("RMB"), CachedMoltenMace, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedHeatShield)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedHeatShield) : false;
			bool bUltimate = CachedHeatShield->IsInUltimateMode();
			bool bBasic = CachedHeatShield->IsBasicAbility();
			Abilities.Add({TEXT("Heat Shield"), TEXT("Q"), CachedHeatShield, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedBlastCharge)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedBlastCharge) : false;
			bool bUltimate = CachedBlastCharge->IsInUltimateMode();
			bool bBasic = CachedBlastCharge->IsBasicAbility();
			Abilities.Add({TEXT("Blast Charge"), TEXT("E"), CachedBlastCharge, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedHammerStrike)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedHammerStrike) : false;
			bool bUltimate = CachedHammerStrike->IsInUltimateMode();
			bool bBasic = CachedHammerStrike->IsBasicAbility();
			Abilities.Add({TEXT("Hammer Strike"), TEXT("R"), CachedHammerStrike, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedForgeDash)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedForgeDash) : false;
			bool bUltimate = CachedForgeDash->IsInUltimateMode();
			bool bBasic = CachedForgeDash->IsBasicAbility();
			Abilities.Add({TEXT("Forge Dash"), TEXT("Shift"), CachedForgeDash, true, bDisabled, bUltimate, bBasic});
		}
		
		if (CachedForgeJump)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedForgeJump) : false;
			bool bUltimate = CachedForgeJump->IsInUltimateMode();
			bool bBasic = CachedForgeJump->IsBasicAbility();
			Abilities.Add({TEXT("Forge Jump"), TEXT("Space"), CachedForgeJump, true, bDisabled, bUltimate, bBasic});
		}
	}
	
	// F slot reserved for both paths
	Abilities.Add({TEXT("(Ultimate)"), TEXT("F"), nullptr, false, false, false, false});
	
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
	
	// Draw ultimate mode indicator if active
	if (bUltimateModeActive)
	{
		float UltX = Canvas->SizeX / 2 - 100;
		float UltY = 150;
		DrawText(TEXT("ULTIMATE MODE ACTIVE"), FColor::Red, UltX, UltY, nullptr, 2.0f);
		DrawText(TEXT("Use an ability to sacrifice it!"), FColor::White, UltX - 20, UltY + 25);
	}
	
	for (int32 i = 0; i < Abilities.Num(); i++)
	{
		const FAbilityDisplayInfo& AbilityInfo = Abilities[i];
		
		float X = StartX + (i % 6) * (BoxWidth + Spacing);
		float Y = StartY + (i / 6) * (BoxHeight + Spacing);
		
		// Draw ability box with different colors for states
		FColor BoxColor = FColor(32, 32, 32); // Default
		FColor NameColor = FColor::White;
		
		if (AbilityInfo.bIsDisabled)
		{
			BoxColor = FColor(16, 16, 16); // Darker for disabled
			NameColor = FColor(128, 128, 128); // Gray text
		}
		else if (AbilityInfo.bIsInUltimateMode)
		{
			// Pulsing effect for ultimate abilities
			float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.0f) * 0.5f + 0.5f;
			BoxColor = FColor(
				FMath::Lerp(128, 255, Pulse),
				FMath::Lerp(0, 128, Pulse),
				FMath::Lerp(0, 128, Pulse)
			);
			NameColor = FColor::Yellow;
		}
		else if (!AbilityInfo.bIsActive)
		{
			BoxColor = FColor(16, 16, 16);
			NameColor = FColor(128, 128, 128);
		}
		
		DrawRect(FColor::Black, X - 2, Y - 2, BoxWidth + 4, BoxHeight + 4);
		DrawRect(BoxColor, X, Y, BoxWidth, BoxHeight);
		
		// Draw ability name
		DrawText(AbilityInfo.Name, NameColor, X + 5, Y + 5);
		
		// Draw disabled overlay
		if (AbilityInfo.bIsDisabled)
		{
			// Draw X over disabled abilities
			DrawLine(X, Y, X + BoxWidth, Y + BoxHeight, FColor::Red, 3.0f);
			DrawLine(X + BoxWidth, Y, X, Y + BoxHeight, FColor::Red, 3.0f);
			DrawText(TEXT("DISABLED"), FColor::Red, X + 20, Y + 35);
		}
		
		// Draw ultimate indicator
		if (AbilityInfo.bIsInUltimateMode && !AbilityInfo.bIsDisabled)
		{
			DrawText(TEXT("ULTIMATE!"), FColor::Yellow, X + 20, Y + 60);
		}
		
		// Draw basic ability indicator
		if (AbilityInfo.bIsBasicAbility)
		{
			DrawText(TEXT("[Basic]"), FColor(100, 100, 255), X + 65, Y + 5);
		}
		
		// Draw input key
		FColor InputColor = AbilityInfo.bIsActive && !AbilityInfo.bIsDisabled ? FColor::Green : FColor(128, 128, 128);
		// Use pre-allocated buffer to avoid per-frame allocation
		FCString::Sprintf(CooldownTextBuffer, TEXT("[%s]"), *AbilityInfo.Input);
		DrawText(CooldownTextBuffer, InputColor, X + 5, Y + 25);
		
		// Draw cooldown and resource cost if ability is active
		if (AbilityInfo.bIsActive && AbilityInfo.Ability)
		{
			// Special handling for HackerJump - show jump cooldown
			if (UHackerJumpAbility* HackerJump = Cast<UHackerJumpAbility>(AbilityInfo.Ability))
			{
				// Check if in air and has jumped
				if (HackerJump->GetCurrentJumpCount() > 0 && HackerJump->GetCurrentJumpCount() < HackerJump->GetMaxJumpCount())
				{
					float JumpCooldownRemaining = HackerJump->GetJumpCooldown() - HackerJump->GetTimeSinceLastJump();
					if (JumpCooldownRemaining > 0.0f)
					{
						// Show jump cooldown
						float CooldownWidth = BoxWidth - 10;
						float CooldownHeight = 5.0f;
						float CooldownPercent = JumpCooldownRemaining / HackerJump->GetJumpCooldown();
						DrawRect(FColor(64, 64, 64), X + 5, Y + 50, CooldownWidth, CooldownHeight);
						DrawRect(FColor::Orange, X + 5, Y + 50, CooldownWidth * CooldownPercent, CooldownHeight);
						// Use pre-allocated buffer to avoid per-frame allocation
						FCString::Sprintf(CooldownTextBuffer, TEXT("Jump CD: %.1fs"), JumpCooldownRemaining);
						DrawText(CooldownTextBuffer, FColor::Orange, X + 5, Y + 60);
					}
					else
					{
						DrawText(TEXT("Jump Ready!"), FColor::Cyan, X + 5, Y + 50);
					}
				}
				else
				{
					DrawText(TEXT("Ready"), FColor::Green, X + 5, Y + 50);
				}
			}
			else
			{
				// Normal cooldown display for other abilities
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
					// Use pre-allocated buffer to avoid per-frame allocation
					FCString::Sprintf(CooldownTextBuffer, TEXT("%.1fs"), TimeRemaining);
					DrawText(CooldownTextBuffer, FColor::Red, X + 5, Y + 60);
				}
				else
				{
					DrawText(TEXT("Ready"), FColor::Green, X + 5, Y + 50);
				}
			}
		}
	}
}

void ABlackholeHUD::OnUltimateModeChanged(bool bActive)
{
	bUltimateModeActive = bActive;
}

void ABlackholeHUD::DrawDebugStatus()
{
	if (!Canvas || !PlayerCharacter)
	{
		return;
	}
	
	float X = Canvas->SizeX - 400.0f;
	float Y = 250.0f;
	float LineHeight = 18.0f;
	
	// Title
	DrawText(TEXT("=== ABILITY STATUS ==="), FColor::Yellow, X, Y);
	Y += LineHeight * 1.5f;
	
	// Ultimate mode status
	const TCHAR* UltimateStatus = bUltimateModeActive ? TEXT("ACTIVE") : TEXT("INACTIVE");
	FColor UltimateColor = bUltimateModeActive ? FColor::Red : FColor::White;
	FCString::Sprintf(DebugTextBuffer, TEXT("Ultimate Mode: %s"), UltimateStatus);
	DrawText(DebugTextBuffer, UltimateColor, X, Y);
	Y += LineHeight;
	
	// Combat status
	if (ThresholdManager)
	{
		bool bInCombat = ThresholdManager->IsInCombat();
		const TCHAR* CombatStatus = bInCombat ? TEXT("IN COMBAT") : TEXT("NOT IN COMBAT");
		FColor CombatColor = bInCombat ? FColor::Orange : FColor(128, 128, 128);
		FCString::Sprintf(DebugTextBuffer, TEXT("Combat: %s"), CombatStatus);
		DrawText(DebugTextBuffer, CombatColor, X, Y);
		Y += LineHeight;
		
		// Disabled abilities count
		int32 DisabledCount = ThresholdManager->GetDisabledAbilityCount();
		FCString::Sprintf(DebugTextBuffer, TEXT("Disabled Abilities: %d"), DisabledCount);
		DrawText(DebugTextBuffer, FColor::Red, X, Y);
		Y += LineHeight * 1.5f;
	}
	
	// Individual ability status
	DrawText(TEXT("Abilities:"), FColor::Cyan, X, Y);
	Y += LineHeight;
	
	TArray<FAbilityDisplayInfo> Abilities = GetCurrentAbilities();
	for (const FAbilityDisplayInfo& AbilityInfo : Abilities)
	{
		if (AbilityInfo.Ability)
		{
			FString Status;
			FColor StatusColor = FColor::White;
			
			if (AbilityInfo.bIsDisabled)
			{
				Status = TEXT("DISABLED");
				StatusColor = FColor::Red;
			}
			else if (AbilityInfo.bIsInUltimateMode)
			{
				Status = TEXT("ULTIMATE");
				StatusColor = FColor::Yellow;
			}
			else if (AbilityInfo.bIsBasicAbility)
			{
				Status = TEXT("BASIC");
				StatusColor = FColor(100, 100, 255);
			}
			else
			{
				// Check if abilities are buffed (50-99% WP)
				if (ResourceManager && ResourceManager->GetWillPowerPercent() >= 0.5f && 
					ResourceManager->GetWillPowerPercent() < 1.0f)
				{
					Status = TEXT("BUFFED");
					StatusColor = FColor::Cyan;
				}
				else
				{
					Status = TEXT("NORMAL");
					StatusColor = FColor::Green;
				}
			}
			
			// Use pre-allocated buffer to avoid per-frame allocation
			FCString::Sprintf(DebugTextBuffer, TEXT("[%s] %s - %s"), 
				*AbilityInfo.Input, 
				*AbilityInfo.Name, 
				*Status);
			DrawText(DebugTextBuffer, StatusColor, X + 10, Y);
			Y += LineHeight;
		}
	}
	
	// WP Status
	Y += LineHeight;
	if (ResourceManager)
	{
		int32 WPPercent = (int32)(ResourceManager->GetWillPowerPercent() * 100.0f);
		DrawText(FString::Printf(TEXT("WP: %d%%"), WPPercent), WillPowerColor, X, Y);
		Y += LineHeight;
		
		if (WPPercent >= 100)
		{
			DrawText(TEXT("USE ABILITY TO SACRIFICE!"), FColor::Red, X, Y);
		}
		else if (WPPercent >= 50)
		{
			DrawText(TEXT("ABILITIES BUFFED"), FColor::Cyan, X, Y);
		}
	}
}

void ABlackholeHUD::DrawComboStatus()
{
	if (!Canvas || !IsValid(CachedComboComponent))
	{
		return;
	}
	
	// Position for combo display (center top)
	float X = Canvas->SizeX * 0.5f - 150.0f;
	float Y = 200.0f;
	
	// Get current combo status
	FString ComboStatus = CachedComboComponent->GetCurrentComboStatus();
	
	if (!ComboStatus.IsEmpty())
	{
		// Draw background box
		float BoxWidth = 300.0f;
		float BoxHeight = 60.0f;
		DrawRect(FColor(0, 0, 0, 180), X - 10, Y - 10, BoxWidth + 20, BoxHeight + 20);
		
		// Determine color based on combo status
		FColor TextColor = FColor::White;
		float TextScale = 1.5f;
		
		if (ComboStatus.Contains(TEXT("COMBO:")))
		{
			// Active combo - make it stand out
			TextColor = FColor::Yellow;
			TextScale = 2.0f;
			
			// Add pulsing effect
			float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) * 0.5f + 0.5f;
			TextColor.R = FMath::Lerp(200, 255, Pulse);
		}
		else if (ComboStatus.Contains(TEXT("Inputs:")))
		{
			// Input sequence - show in cyan
			TextColor = FColor::Cyan;
		}
		
		// Draw the combo text
		DrawText(ComboStatus, TextColor, X, Y, nullptr, TextScale);
		
		// Add visual effects for specific combos
		if (ComboStatus.Contains(TEXT("COMBO: PhantomStrike")))
		{
			// Dash + Slash visual
			DrawText(TEXT("💨⚔️ PHANTOM STRIKE!"), FColor::Cyan, X - 50, Y + 35, nullptr, 1.2f);
			DrawText(TEXT("Teleport Backstab Critical Hit!"), FColor::White, X - 70, Y + 55, nullptr, 0.9f);
		}
		else if (ComboStatus.Contains(TEXT("COMBO: BladeDance")))
		{
			// Slash + Slash visual
			DrawText(TEXT("⚔️⚔️ BLADE DANCE!"), FColor::Red, X - 50, Y + 35, nullptr, 1.2f);
			DrawText(TEXT("Progressive Combo Damage!"), FColor::White, X - 60, Y + 55, nullptr, 0.9f);
		}
		else if (ComboStatus.Contains(TEXT("COMBO: AerialRave")))
		{
			// Jump + Slash visual
			DrawText(TEXT("🚀⚔️ AERIAL RAVE!"), FColor::Green, X - 50, Y + 35, nullptr, 1.2f);
			DrawText(TEXT("Shockwave Ground Slam!"), FColor::White, X - 60, Y + 55, nullptr, 0.9f);
		}
		else if (ComboStatus.Contains(TEXT("COMBO: TempestBlade")))
		{
			// Jump + Dash + Slash visual
			DrawText(TEXT("🌪️⚔️ TEMPEST BLADE!"), FColor::Purple, X - 50, Y + 35, nullptr, 1.2f);
			DrawText(TEXT("Multi-Teleport Strike!"), FColor::White, X - 50, Y + 55, nullptr, 0.9f);
		}
		
		// Draw combo hints below if showing inputs
		if (ComboStatus.Contains(TEXT("Inputs:")))
		{
			FString HintText = TEXT("Combos: Dash+Slash | Jump+Slash | Jump+Dash+Slash | Slash+Slash");
			DrawText(HintText, FColor(150, 150, 150), X - 50, Y + 30, nullptr, 0.8f);
		}
	}
	
	// Show last performed combo if recent
	FName LastCombo = CachedComboComponent->GetLastPerformedCombo();
	if (LastCombo != NAME_None)
	{
		// This will be shown through the ComboStatus string above
		// Additional visual feedback could be added here if needed
	}
}