#include "UI/BlackholeHUD.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Attributes/IntegrityComponent.h"
#include "Components/Attributes/StaminaComponent.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Systems/ResourceManager.h"
#include "UI/MainMenuWidget.h"
#include "UI/PauseMenuWidget.h"
#include "UI/GameOverWidget.h"
#include "UI/SimplePauseMenu.h"
#include "Components/Abilities/Player/Basic/SlashAbilityComponent.h"
// #include "Components/Abilities/Player/SystemFreezeAbilityComponent.h" // Removed
#include "Components/Abilities/Player/Basic/KillAbilityComponent.h"
#include "Components/Abilities/Player/Hacker/GravityPullAbilityComponent.h"
#include "Components/Abilities/Player/Utility/HackerDashAbility.h"
#include "Components/Abilities/Player/Utility/HackerJumpAbility.h"
#include "Components/Abilities/UtilityAbility.h"
#include "Components/Abilities/Player/Hacker/PulseHackAbility.h"
#include "Components/Abilities/Player/Hacker/FirewallBreachAbility.h"
#include "Components/Abilities/Player/Hacker/DataSpikeAbility.h"
#include "Components/Abilities/Player/Hacker/SystemOverrideAbility.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Systems/ThresholdManager.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"

ABlackholeHUD::ABlackholeHUD()
{
	AttributeBarWidth = 200.0f;
	AttributeBarHeight = 20.0f;
	CooldownIconSize = 50.0f;

	IntegrityColor = FColor::Red;
	StaminaColor = FColor::Green;
	WillPowerColor = FColor::Blue;
	
	// Initialize cached values
	CachedWP = 100.0f;
	CachedMaxWP = 100.0f;
	bUltimateModeActive = false;
}

void ABlackholeHUD::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<ABlackholePlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	// Get GameStateManager and bind to state changes
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		GameStateManager = GameInstance->GetSubsystem<UGameStateManager>();
		if (GameStateManager)
		{
			GameStateManager->OnGameStateChanged.AddDynamic(this, &ABlackholeHUD::OnGameStateChanged);
		}
	}
	
	// Create menu widgets directly from C++ for prototyping
	// No blueprint classes needed
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		// Use simplified pause menu for prototyping
		SimplePauseMenu = USimplePauseMenu::CreateSimplePauseMenu(PC);
		
		// Create other menus normally
		MainMenuWidget = CreateWidget<UMainMenuWidget>(PC, UMainMenuWidget::StaticClass());
		PauseMenuWidget = CreateWidget<UPauseMenuWidget>(PC, UPauseMenuWidget::StaticClass());
		GameOverWidget = CreateWidget<UGameOverWidget>(PC, UGameOverWidget::StaticClass());
		
		// Log widget creation
		UE_LOG(LogTemp, Log, TEXT("Created menu widgets - SimplePause: %s, MainMenu: %s, PauseMenu: %s, GameOver: %s"),
			SimplePauseMenu ? TEXT("Valid") : TEXT("NULL"),
			MainMenuWidget ? TEXT("Valid") : TEXT("NULL"),
			PauseMenuWidget ? TEXT("Valid") : TEXT("NULL"),
			GameOverWidget ? TEXT("Valid") : TEXT("NULL"));
	}
	
	// For prototyping: Start directly in Playing state instead of MainMenu
	if (GameStateManager && GameStateManager->GetCurrentState() == EGameState::MainMenu)
	{
		UE_LOG(LogTemp, Log, TEXT("Starting game directly in Playing state for prototyping"));
		GameStateManager->StartGame();
	}
	
	// Set up input handling for ESC key
	SetupInputComponent();
	
	// Skip showing menus on startup for prototyping
	// The game will start directly in Playing state
	
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
		CachedDataSpike = PlayerCharacter->FindComponentByClass<UDataSpikeAbility>();
		CachedSystemOverride = PlayerCharacter->FindComponentByClass<USystemOverrideAbility>();
		CachedHackerDash = PlayerCharacter->FindComponentByClass<UHackerDashAbility>();
		CachedHackerJump = PlayerCharacter->FindComponentByClass<UHackerJumpAbility>();
		
	}
	
	// Bind to ResourceManager events
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		ResourceManager = GameInstance->GetSubsystem<UResourceManager>();
		if (ResourceManager)
		{
			ResourceManager->OnWillPowerChanged.AddDynamic(this, &ABlackholeHUD::UpdateWPBar);
			
			// Initialize with current values
			CachedWP = ResourceManager->GetCurrentWillPower();
			CachedMaxWP = ResourceManager->GetMaxWillPower();
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

void ABlackholeHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind ResourceManager delegates
	if (ResourceManager)
	{
		ResourceManager->OnWillPowerChanged.RemoveDynamic(this, &ABlackholeHUD::UpdateWPBar);
		ResourceManager = nullptr;
	}
	
	// Unbind ThresholdManager delegates
	if (ThresholdManager)
	{
		ThresholdManager->OnUltimateModeActivated.RemoveDynamic(this, &ABlackholeHUD::OnUltimateModeChanged);
		ThresholdManager = nullptr;
	}
	
	// Unbind GameStateManager delegates
	if (GameStateManager)
	{
		GameStateManager->OnGameStateChanged.RemoveDynamic(this, &ABlackholeHUD::OnGameStateChanged);
		GameStateManager = nullptr;
	}
	
	// Clean up menu widgets
	HideAllMenus();
	if (MainMenuWidget) MainMenuWidget = nullptr;
	if (PauseMenuWidget) PauseMenuWidget = nullptr;
	if (SimplePauseMenu) SimplePauseMenu = nullptr;
	if (GameOverWidget) GameOverWidget = nullptr;
	
	// Clear all cached ability pointers
	PlayerCharacter = nullptr;
	CachedSlashAbility = nullptr;
	CachedKillAbility = nullptr;
	CachedFirewallBreach = nullptr;
	CachedPulseHack = nullptr;
	CachedGravityPull = nullptr;
	CachedDataSpike = nullptr;
	CachedSystemOverride = nullptr;
	CachedHackerDash = nullptr;
	CachedHackerJump = nullptr;
	
	Super::EndPlay(EndPlayReason);
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
	// Always draw Will Power for Hacker path
	DrawAttribute("Will Power", CachedWP, CachedMaxWP, 
				  StartX, StartY + VerticalSpacing * resourceBarIndex, WillPowerColor);
	resourceBarIndex++;
	
	// Path is always Hacker now
	FString PathText = TEXT("Path: Hacker");
	DrawText(PathText, FColor::Cyan, StartX, StartY + VerticalSpacing * resourceBarIndex, nullptr, 1.0f, false);

	// Draw all abilities
	DrawAbilityInfo();
	
	// Draw debug status on the right
	DrawDebugStatus();

	DrawTargetInfo();
}

void ABlackholeHUD::DrawAttribute(const FString& Name, float Current, float Max, float X, float Y, const FColor& Color)
{
	FFontRenderInfo RenderInfo;
	DrawText(Name, Color, X, Y, nullptr, 1.0f, false);

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
			DrawText(TEXT("ULTIMATE!"), FColor::Red, BarX + AttributeBarWidth / 2 - 30, BarY - 15, nullptr, 1.0f, false);
		}
		else if (Current >= Max * 0.5f)
		{
			DrawText(TEXT("BUFFED"), FColor::Cyan, BarX + AttributeBarWidth / 2 - 20, BarY - 15, nullptr, 1.0f, false);
		}
	}
	else
	{
		DrawRect(Color, BarX, BarY, FilledWidth, AttributeBarHeight);
	}

	// Use pre-allocated buffer to avoid per-frame allocation
	FCString::Sprintf(AttributeTextBuffer, TEXT("%.0f/%.0f"), Current, Max);
	DrawText(AttributeTextBuffer, FColor::White, BarX + AttributeBarWidth + 10, Y, nullptr, 1.0f, false);
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


TArray<ABlackholeHUD::FAbilityDisplayInfo> ABlackholeHUD::GetCurrentAbilities() const
{
	TArray<FAbilityDisplayInfo> Abilities;
	
	if (!PlayerCharacter)
	{
		return Abilities;
	}
	
	bool bIsHacker = true; // Always Hacker path now
	
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
		
		// Data Spike ability (R key)
		if (CachedDataSpike)
		{
			bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedDataSpike) : false;
			bool bUltimate = CachedDataSpike->IsInUltimateMode();
			bool bBasic = CachedDataSpike->IsBasicAbility();
			FString Name = bUltimate ? TEXT("SYSTEM CORRUPTION") : TEXT("Data Spike");
			Abilities.Add({Name, TEXT("R"), CachedDataSpike, true, bDisabled, bUltimate, bBasic});
		}
	}
	
	// System Override ability (F key for Hacker)
	if (CachedSystemOverride)
	{
		bool bDisabled = ThresholdManager ? ThresholdManager->IsAbilityDisabled(CachedSystemOverride) : false;
		bool bUltimate = CachedSystemOverride->IsInUltimateMode();
		bool bBasic = CachedSystemOverride->IsBasicAbility();
		FString Name = bUltimate ? TEXT("TOTAL SYSTEM SHUTDOWN") : TEXT("System Override");
		Abilities.Add({Name, TEXT("F"), CachedSystemOverride, true, bDisabled, bUltimate, bBasic});
	}
	
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

void ABlackholeHUD::SetupInputComponent()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (UInputComponent* PlayerInputComponent = PC->InputComponent)
		{
			PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &ABlackholeHUD::OnEscapePressed);
		}
	}
}

void ABlackholeHUD::OnEscapePressed()
{
	if (!GameStateManager)
	{
		return;
	}
	
	EGameState CurrentState = GameStateManager->GetCurrentState();
	
	switch (CurrentState)
	{
	case EGameState::Playing:
		GameStateManager->PauseGame();
		ShowPauseMenu();
		break;
		
	case EGameState::Paused:
		GameStateManager->ResumeGame();
		HideAllMenus();
		break;
		
	case EGameState::GameOver:
		// In game over state, ESC restarts the game
		HideAllMenus();
		GameStateManager->ResetGame();
		GameStateManager->StartGame();
		break;
		
	case EGameState::MainMenu:
		// In main menu, ESC quits the game
		GameStateManager->QuitGame();
		break;
		
	default:
		break;
	}
}

void ABlackholeHUD::ShowMainMenu()
{
	HideAllMenus();
	
	if (MainMenuWidget)
	{
		MainMenuWidget->ShowMenu();
	}
	else
	{
		// If no menu widget exists, ensure game input is enabled
		UE_LOG(LogTemp, Warning, TEXT("No MainMenuWidget available - enabling game input"));
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
	}
}

void ABlackholeHUD::ShowPauseMenu()
{
	HideAllMenus();
	
	UE_LOG(LogTemp, Log, TEXT("ShowPauseMenu called - SimplePauseMenu: %s"), 
		SimplePauseMenu ? TEXT("Valid") : TEXT("NULL"));
	
	// Use simplified pause menu for prototyping
	if (SimplePauseMenu)
	{
		SimplePauseMenu->ShowMenu();
		UE_LOG(LogTemp, Log, TEXT("SimplePauseMenu->ShowMenu() called"));
	}
	else if (PauseMenuWidget)
	{
		PauseMenuWidget->ShowMenu();
		UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget->ShowMenu() called"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No pause menu widget available!"));
	}
}

void ABlackholeHUD::ShowGameOverMenu()
{
	HideAllMenus();
	
	if (GameOverWidget)
	{
		GameOverWidget->ShowMenu();
	}
}

void ABlackholeHUD::HideAllMenus()
{
	if (MainMenuWidget && MainMenuWidget->IsInViewport())
	{
		MainMenuWidget->HideMenu();
	}
	
	if (SimplePauseMenu && SimplePauseMenu->IsInViewport())
	{
		SimplePauseMenu->HideMenu();
	}
	
	if (PauseMenuWidget && PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->HideMenu();
	}
	
	if (GameOverWidget && GameOverWidget->IsInViewport())
	{
		GameOverWidget->HideMenu();
	}
}

void ABlackholeHUD::OnMenuTogglePressed()
{
	if (!GameStateManager)
	{
		return;
	}
	
	EGameState CurrentState = GameStateManager->GetCurrentState();
	
	// Only allow menu toggle in Playing or Paused states
	switch (CurrentState)
	{
	case EGameState::Playing:
		// Pause the game
		GameStateManager->PauseGame();
		break;
		
	case EGameState::Paused:
		// Resume the game
		GameStateManager->ResumeGame();
		break;
		
	default:
		// Don't toggle menu in other states (MainMenu, GameOver, etc.)
		break;
	}
}

void ABlackholeHUD::OnGameStateChanged(EGameState NewState)
{
	switch (NewState)
	{
	case EGameState::MainMenu:
		ShowMainMenu();
		// If no main menu widget, automatically start the game
		if (!MainMenuWidget && GameStateManager)
		{
			UE_LOG(LogTemp, Warning, TEXT("No MainMenuWidget - auto-starting game"));
			// Delay to next frame to avoid state change during state change
			GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
			{
				if (GameStateManager)
				{
					GameStateManager->StartGame();
				}
			});
		}
		break;
		
	case EGameState::Playing:
		HideAllMenus();
		// Ensure game input mode - delay slightly to ensure proper setup
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick([this]()
			{
				if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
				{
					// Force unpause in case we're still paused
					PC->SetPause(false);
					
					FInputModeGameOnly InputMode;
					PC->SetInputMode(InputMode);
					PC->bShowMouseCursor = false;
					
					UE_LOG(LogTemp, Log, TEXT("BlackholeHUD: Set input mode to GameOnly for Playing state"));
				}
			});
		}
		break;
		
	case EGameState::Paused:
		ShowPauseMenu();
		break;
		
	case EGameState::GameOver:
		ShowGameOverMenu();
		break;
		
	default:
		HideAllMenus();
		break;
	}
}