#include "UI/BlackholeHUD.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Attributes/WillPowerComponent.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
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
#include "Components/Movement/WallRunComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"

ABlackholeHUD::ABlackholeHUD()
{
	AttributeBarWidth = 200.0f;
	AttributeBarHeight = 20.0f;
	CooldownIconSize = 50.0f;

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
				ThresholdManager->OnCriticalTimer.AddDynamic(this, &ABlackholeHUD::OnCriticalTimerUpdate);
				ThresholdManager->OnCriticalTimerExpired.AddDynamic(this, &ABlackholeHUD::OnCriticalTimerExpired);
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
	
	// Update cached values if needed
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastCacheUpdateTime >= CacheUpdateInterval)
	{
		UpdateCachedValues();
		LastCacheUpdateTime = CurrentTime;
	}
	
	// Update notifications
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	UpdateNotifications(DeltaTime);

	DrawCrosshair();

	float StartX = 50.0f;
	float StartY = 50.0f;
	float VerticalSpacing = 30.0f;

	// Draw WP as the primary resource (energy system)
	int resourceBarIndex = 0;
	// Calculate dynamic color based on WP level
	float WPPercent = CachedMaxWP > 0 ? CachedWP / CachedMaxWP : 0.0f;
	FColor DynamicWPColor;
	if (WPPercent >= 0.5f)
	{
		// 50-100% = Green (healthy)
		DynamicWPColor = FColor::Green;
	}
	else if (WPPercent >= 0.2f)
	{
		// 20-50% = Yellow (warning)
		DynamicWPColor = FColor::Yellow;
	}
	else
	{
		// 0-20% = Red (critical)
		DynamicWPColor = FColor::Red;
	}
	DrawAttribute("Will Power", CachedWP, CachedMaxWP, 
				  StartX, StartY + VerticalSpacing * resourceBarIndex, DynamicWPColor);
	resourceBarIndex++;
	
	// Path is always Hacker now
	FString PathText = TEXT("Path: Hacker");
	DrawText(PathText, FColor::Cyan, StartX, StartY + VerticalSpacing * resourceBarIndex, nullptr, 1.0f, false);
	resourceBarIndex++;
	
	// Draw critical state entries remaining
	if (ThresholdManager)
	{
		int32 EntriesRemaining = ThresholdManager->GetCriticalStateEntriesRemaining();
		int32 TotalEntries = ThresholdManager->GetCriticalStateLimit();
		FColor EntryColor = EntriesRemaining > 0 ? FColor::White : FColor::Red;
		if (EntriesRemaining == 1)
		{
			EntryColor = FColor::Yellow; // Warning color for last entry
		}
		FString CriticalText = FString::Printf(TEXT("Critical Entries: %d/%d"), EntriesRemaining, TotalEntries);
		DrawText(CriticalText, EntryColor, StartX, StartY + VerticalSpacing * resourceBarIndex, nullptr, 1.0f, false);
	}

	// Draw all abilities
	DrawAbilityInfo();
	
	// Draw debug status on the right
	DrawDebugStatus();
	
	// Draw critical timer if active
	DrawCriticalTimer();
	
	// Draw wall run timer if active
	DrawWallRunTimer();
	
	// Velocity indicator removed per user request
	// DrawVelocityIndicator();
	
	// Draw notifications
	DrawNotifications();
	
	// Draw enemy ability progress bars
	DrawEnemyAbilityProgress();

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
			// Use the already calculated dynamic color
			DrawRect(Color, BarX, BarY, FilledWidth, AttributeBarHeight);
		}
		
		// Draw threshold markers
		float Threshold50 = BarX + (0.5f * AttributeBarWidth);
		DrawLine(Threshold50, BarY - 2, Threshold50, BarY + AttributeBarHeight + 2, FColor::Yellow, 2.0f);
		
		// Draw threshold text
		if (Current <= Max * 0.2f)
		{
			DrawText(TEXT("LOW ENERGY"), FColor::Red, BarX + AttributeBarWidth / 2 - 30, BarY - 15, nullptr, 1.0f, false);
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

	// Display enemy WP
	if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(Target))
	{
		DrawAttribute("Enemy WP", Enemy->GetCurrentWP(), Enemy->GetMaxWP(),
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

void ABlackholeHUD::UpdateCachedValues()
{
	if (!PlayerCharacter)
	{
		return;
	}
	
	// Update WP values
	if (ResourceManager)
	{
		CachedCurrentWP = ResourceManager->GetCurrentWillPower();
		CachedMaxWP = ResourceManager->GetMaxWillPower();
		CachedWPPercent = ResourceManager->GetWillPowerPercent();
	}
	
	// Update ultimate mode status
	if (ThresholdManager)
	{
		bIsUltimateMode = ThresholdManager->IsUltimateModeActive();
	}
	
	// Update ability cooldowns
	CachedCooldownPercents.Empty();
	TArray<FAbilityDisplayInfo> Abilities = GetCurrentAbilities();
	for (const FAbilityDisplayInfo& AbilityInfo : Abilities)
	{
		if (AbilityInfo.Ability)
		{
			CachedCooldownPercents.Add(AbilityInfo.Ability->GetCooldownPercentage());
		}
		else
		{
			CachedCooldownPercents.Add(0.0f);
		}
	}
	
	// Update velocity values
	if (UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement())
	{
		CachedVelocityVector = Movement->Velocity;
		CachedSpeed = CachedVelocityVector.Size();
		CachedVelocity = CachedVelocityVector.Size2D(); // Horizontal velocity only
	}
	
	// Update movement states
	if (UWallRunComponent* WallRunComp = PlayerCharacter->GetWallRunComponent())
	{
		bIsWallRunning = WallRunComp->IsWallRunning();
	}
	
	// Check if dashing - you could check if dash ability is active
	if (CachedHackerDash)
	{
		bIsDashing = CachedHackerDash->IsOnCooldown() && CachedHackerDash->GetCooldownPercentage() > 0.9f;
	}
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

void ABlackholeHUD::DrawVelocityIndicator()
{
	if (!PlayerCharacter || !Canvas)
	{
		return;
	}
	
	// Position for velocity indicator - bottom center of screen
	float ScreenCenterX = Canvas->SizeX * 0.5f;
	float ScreenBottomY = Canvas->SizeY - 150.0f;
	
	// Draw background box for velocity display
	float BoxWidth = 200.0f;
	float BoxHeight = 80.0f;
	float BoxX = ScreenCenterX - BoxWidth * 0.5f;
	float BoxY = ScreenBottomY - BoxHeight;
	
	// Semi-transparent background
	DrawRect(FColor(0, 0, 0, 150), BoxX, BoxY, BoxWidth, BoxHeight);
	
	// Draw border for the box
	float BorderThickness = 2.0f;
	FColor BorderColor = FColor(100, 100, 100, 255);
	// Top border
	DrawRect(BorderColor, BoxX, BoxY, BoxWidth, BorderThickness);
	// Bottom border
	DrawRect(BorderColor, BoxX, BoxY + BoxHeight - BorderThickness, BoxWidth, BorderThickness);
	// Left border
	DrawRect(BorderColor, BoxX, BoxY, BorderThickness, BoxHeight);
	// Right border
	DrawRect(BorderColor, BoxX + BoxWidth - BorderThickness, BoxY, BorderThickness, BoxHeight);
	
	// Calculate speed color based on velocity
	FColor SpeedColor = FColor::White;
	if (CachedVelocity > 3000.0f) // Dash speed or higher
	{
		SpeedColor = FColor::Purple; // Ultra speed
	}
	else if (CachedVelocity > 1500.0f) // Wall run speed
	{
		SpeedColor = FColor::Cyan; // High speed
	}
	else if (CachedVelocity > 600.0f) // Normal run speed
	{
		SpeedColor = FColor::Green; // Normal speed
	}
	else if (CachedVelocity > 100.0f) // Walking
	{
		SpeedColor = FColor::Yellow; // Slow
	}
	else
	{
		SpeedColor = FColor::Red; // Stationary/very slow
	}
	
	// Draw main speed value
	FString SpeedText = FString::Printf(TEXT("%.0f"), CachedVelocity);
	float TextScale = 1.8f;  // Smaller text
	DrawText(SpeedText, SpeedColor, ScreenCenterX - 25.0f, BoxY + 5.0f, nullptr, TextScale, false);
	
	// Draw speed label
	DrawText(TEXT("SPEED"), FColor(128, 128, 128), ScreenCenterX - 20.0f, BoxY + 30.0f, nullptr, 0.8f, false);
	
	// Draw velocity bar
	float BarY = BoxY + 48.0f;
	float BarWidth = 160.0f;  // Smaller bar
	float BarHeight = 6.0f;
	float BarX = BoxX + 10.0f;
	
	// Background bar
	DrawRect(FColor(50, 50, 50, 255), BarX, BarY, BarWidth, BarHeight);
	
	// Fill bar based on speed (max out at 4000 for display)
	float MaxDisplaySpeed = 4000.0f;
	float SpeedPercent = FMath::Clamp(CachedVelocity / MaxDisplaySpeed, 0.0f, 1.0f);
	float FilledWidth = BarWidth * SpeedPercent;
	
	// Gradient color for bar
	FLinearColor LinearRed(FColor::Red);
	FLinearColor LinearSpeedColor(SpeedColor);
	FLinearColor LerpedColor = FLinearColor::LerpUsingHSV(LinearRed, LinearSpeedColor, SpeedPercent);
	FColor BarColor = LerpedColor.ToFColor(true);
	DrawRect(BarColor, BarX, BarY, FilledWidth, BarHeight);
}

void ABlackholeHUD::DrawAbilityInfo()
{
	if (!Canvas)
	{
		return;
	}
	
	// Use cached abilities if cache is fresh
	TArray<FAbilityDisplayInfo> Abilities;
	if (GetWorld()->GetTimeSeconds() - LastCacheUpdateTime < CacheUpdateInterval)
	{
		// Cache is fresh, reuse the abilities from cache
		Abilities = GetCurrentAbilities();
	}
	else
	{
		// Cache is stale, get fresh data
		Abilities = GetCurrentAbilities();
	}
	
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
				// Check if abilities are buffed (50-100% WP)
				if (ResourceManager && ResourceManager->GetWillPowerPercent() >= 0.5f)
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
		
		if (WPPercent <= 0)
		{
			DrawText(TEXT("ULTIMATE MODE ACTIVE!"), FColor::Cyan, X, Y);
		}
		else if (WPPercent <= 20)
		{
			DrawText(TEXT("LOW ENERGY - USE COMBOS!"), FColor::Red, X, Y);
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

// Critical Timer Functions
void ABlackholeHUD::OnCriticalTimerUpdate(float TimeRemaining)
{
	if (TimeRemaining <= 0.0f)
	{
		// Timer stopped or expired
		bCriticalTimerActive = false;
		CriticalTimeRemaining = 0.0f;
	}
	else
	{
		// Timer active and counting down
		bCriticalTimerActive = true;
		CriticalTimeRemaining = TimeRemaining;
	}
}

void ABlackholeHUD::OnCriticalTimerExpired()
{
	bCriticalTimerActive = false;
	CriticalTimeRemaining = 0.0f;
	UE_LOG(LogTemp, Warning, TEXT("BlackholeHUD: Critical timer expired - hiding UI"));
}

void ABlackholeHUD::DrawCriticalTimer()
{
	if (!bCriticalTimerActive || !Canvas)
	{
		return;
	}
	
	// Get screen center
	float ScreenCenterX = Canvas->SizeX * 0.5f;
	float ScreenCenterY = Canvas->SizeY * 0.3f; // Upper center of screen
	
	// Draw flashing "CRITICAL ERROR" text
	float FlashIntensity = FMath::Sin(GetWorld()->GetTimeSeconds() * 8.0f) * 0.5f + 0.5f; // Flash between 0.5 and 1.0
	FColor CriticalColor = FColor(255, (uint8)(100 * FlashIntensity), 0, 255); // Orange to red flash
	
	FString CriticalText = TEXT("CRITICAL ERROR");
	DrawText(CriticalText, CriticalColor, ScreenCenterX - 150.0f, ScreenCenterY - 40.0f, nullptr, 2.0f, false);
	
	// Draw countdown timer
	FString TimerText = FString::Printf(TEXT("USE ULTIMATE IN: %.1f"), CriticalTimeRemaining);
	FColor TimerColor = CriticalTimeRemaining <= 2.0f ? FColor::Red : FColor::Yellow;
	DrawText(TimerText, TimerColor, ScreenCenterX - 120.0f, ScreenCenterY, nullptr, 1.5f, false);
	
	// Draw warning message
	FString WarningText = TEXT("USE ANY ULTIMATE ABILITY OR DIE!");
	DrawText(WarningText, FColor::White, ScreenCenterX - 140.0f, ScreenCenterY + 30.0f, nullptr, 1.0f, false);
	
	// Draw critical state entries info
	if (ThresholdManager)
	{
		int32 EntriesRemaining = ThresholdManager->GetCriticalStateEntriesRemaining();
		FColor EntryColor = EntriesRemaining > 0 ? FColor::Yellow : FColor::Red;
		FString EntryText = FString::Printf(TEXT("Critical Entries Remaining: %d"), EntriesRemaining);
		DrawText(EntryText, EntryColor, ScreenCenterX - 120.0f, ScreenCenterY + 50.0f, nullptr, 1.2f, false);
		
		// Additional warning if this is the last entry
		if (EntriesRemaining == 0)
		{
			FString LastChanceText = TEXT("LAST CHANCE - NO MORE RETRIES!");
			DrawText(LastChanceText, FColor::Red, ScreenCenterX - 130.0f, ScreenCenterY + 70.0f, nullptr, 1.0f, false);
		}
	}
	
	// Draw flashing border effect
	if (FlashIntensity > 0.7f)
	{
		// Draw red border around screen
		FColor BorderColor = FColor(255, 0, 0, (uint8)(FlashIntensity * 100));
		
		// Top border
		DrawRect(BorderColor, 0, 0, Canvas->SizeX, 10);
		// Bottom border
		DrawRect(BorderColor, 0, Canvas->SizeY - 10, Canvas->SizeX, 10);
		// Left border
		DrawRect(BorderColor, 0, 0, 10, Canvas->SizeY);
		// Right border
		DrawRect(BorderColor, Canvas->SizeX - 10, 0, 10, Canvas->SizeY);
	}
}

void ABlackholeHUD::DrawWallRunTimer()
{
	if (!PlayerCharacter || !Canvas)
	{
		return;
	}
	
	// Get wall run component
	UWallRunComponent* WallRunComp = PlayerCharacter->GetWallRunComponent();
	if (!WallRunComp)
	{
		// No wall run component found
		return;
	}
	
	// Only show wall run UI when actively wall running
	if (!WallRunComp->IsWallRunning())
	{
		return;
	}
	
	// Extra safety check - ensure we have a valid wall side
	if (WallRunComp->GetCurrentWallSide() == EWallSide::None)
	{
		return;
	}
	
	// Drawing wall run UI
	
	// Draw wall run status in a more visible position
	float ScreenRightX = FMath::Max(Canvas->SizeX - 350.0f, Canvas->SizeX * 0.65f); // Ensure it's not too far right
	float ScreenTopY = 200.0f; // Lower position to avoid overlap with other UI
	
	// Draw background box for better visibility
	float BoxWidth = 250.0f;
	float BoxHeight = 80.0f;
	DrawRect(FColor(0, 0, 0, 150), ScreenRightX - 10.0f, ScreenTopY - 10.0f, BoxWidth, BoxHeight);
	
	// Draw wall run indicator
	FString WallRunText = TEXT("WALL RUNNING");
	FColor WallRunColor = FColor::Cyan;
	DrawText(WallRunText, WallRunColor, ScreenRightX, ScreenTopY, nullptr, 1.5f, false);
	
	// Get wall side for display
	FString SideText = "";
	FColor SideColor = FColor::Blue;
	
	if (WallRunComp->GetCurrentWallSide() == EWallSide::Right)
	{
		SideText = " (RIGHT WALL)";
		SideColor = FColor::Green;
	}
	else if (WallRunComp->GetCurrentWallSide() == EWallSide::Left)
	{
		SideText = " (LEFT WALL)";
		SideColor = FColor::Orange;
	}
	
	// Draw wall side text
	FString WallSideText = FString::Printf(TEXT("Running on%s"), *SideText);
	DrawText(WallSideText, SideColor, ScreenRightX, ScreenTopY + 25.0f, nullptr, 1.2f, false);
	
	// Draw control hints
	FString HintText = TEXT("Hold W: Continue | SPACE: Wall Jump (diagonal)");
	DrawText(HintText, FColor::Yellow, ScreenRightX, ScreenTopY + 50.0f, nullptr, 1.0f, false);
	
	// Draw screen edge effects for better visibility
	float EdgeThickness = 15.0f;
	FColor EdgeColor = FColor(0, 255, 255, 100); // Cyan with transparency
	
	// Top edge
	DrawRect(EdgeColor, 0, 0, Canvas->SizeX, EdgeThickness);
	// Bottom edge
	DrawRect(EdgeColor, 0, Canvas->SizeY - EdgeThickness, Canvas->SizeX, EdgeThickness);
	
	// Side edges based on wall side
	if (WallRunComp->GetCurrentWallSide() == EWallSide::Right)
	{
		// Right edge highlight
		DrawRect(FColor(0, 255, 0, 150), Canvas->SizeX - EdgeThickness, 0, EdgeThickness, Canvas->SizeY);
	}
	else if (WallRunComp->GetCurrentWallSide() == EWallSide::Left)
	{
		// Left edge highlight
		DrawRect(FColor(255, 128, 0, 150), 0, 0, EdgeThickness, Canvas->SizeY);
	}
	
	// Show current speed for feedback
	if (PlayerCharacter && PlayerCharacter->GetCharacterMovement())
	{
		float CurrentSpeed = PlayerCharacter->GetCharacterMovement()->Velocity.Size2D();
		FString SpeedText = FString::Printf(TEXT("Speed: %.0f"), CurrentSpeed);
		DrawText(SpeedText, FColor::White, ScreenRightX, ScreenTopY + 75.0f, nullptr, 1.0f, false);
	}
}

// Notification System Implementation
void ABlackholeHUD::AddNotification(const FString& Message, const FColor& Color, float Duration)
{
	FNotification NewNotification;
	NewNotification.Message = Message;
	NewNotification.Color = Color;
	NewNotification.TimeRemaining = Duration;
	NewNotification.TotalDuration = Duration;
	
	ActiveNotifications.Add(NewNotification);
	
	// Limit the number of notifications to prevent spam
	if (ActiveNotifications.Num() > 5)
	{
		ActiveNotifications.RemoveAt(0);
	}
}

void ABlackholeHUD::UpdateNotifications(float DeltaTime)
{
	// Update notification timers and remove expired ones
	for (int32 i = ActiveNotifications.Num() - 1; i >= 0; i--)
	{
		ActiveNotifications[i].TimeRemaining -= DeltaTime;
		if (ActiveNotifications[i].TimeRemaining <= 0.0f)
		{
			ActiveNotifications.RemoveAt(i);
		}
	}
	
	// Update enemy ability progress timers
	if (bShowingPsiDisruptorProgress)
	{
		PsiDisruptorBuildProgress += DeltaTime;
		if (PsiDisruptorBuildProgress >= PsiDisruptorBuildTime)
		{
			bShowingPsiDisruptorProgress = false;
			PsiDisruptorBuildProgress = 0.0f;
		}
	}
	
	if (bShowingMindmeldProgress)
	{
		MindmeldProgress += DeltaTime;
		if (MindmeldProgress >= MindmeldCastTime)
		{
			bShowingMindmeldProgress = false;
			MindmeldProgress = 0.0f;
			MindmeldCaster = nullptr;
		}
		
		// Clear caster if it becomes invalid
		if (MindmeldCaster && !IsValid(MindmeldCaster))
		{
			MindmeldCaster = nullptr;
		}
	}
}

void ABlackholeHUD::DrawNotifications()
{
	if (!Canvas || ActiveNotifications.Num() == 0)
	{
		return;
	}
	
	float NotificationX = Canvas->SizeX * 0.5f - 200.0f;
	float NotificationY = Canvas->SizeY * 0.2f;
	float NotificationHeight = 30.0f;
	float NotificationSpacing = 5.0f;
	
	for (int32 i = 0; i < ActiveNotifications.Num(); i++)
	{
		const FNotification& Notification = ActiveNotifications[i];
		
		// Calculate fade alpha based on time remaining
		float Alpha = 1.0f;
		if (Notification.TimeRemaining < 1.0f)
		{
			Alpha = Notification.TimeRemaining;
		}
		
		// Draw notification background
		FColor BGColor = FColor(0, 0, 0, (uint8)(150 * Alpha));
		DrawRect(BGColor, NotificationX - 10, NotificationY + i * (NotificationHeight + NotificationSpacing) - 5,
				420, NotificationHeight);
		
		// Draw notification text
		FColor TextColor = Notification.Color;
		TextColor.A = (uint8)(255 * Alpha);
		DrawText(Notification.Message, TextColor, NotificationX, 
				NotificationY + i * (NotificationHeight + NotificationSpacing), nullptr, 1.2f, false);
	}
}

void ABlackholeHUD::DrawEnemyAbilityProgress()
{
	if (!Canvas)
	{
		return;
	}
	
	// Move progress bars to top right corner
	float ProgressBarWidth = 300.0f;
	float ProgressBarHeight = 30.0f;
	float ProgressBarX = Canvas->SizeX - ProgressBarWidth - 20.0f; // 20 pixels from right edge
	float ProgressBarY = 100.0f; // Below the top HUD elements
	
	// Draw Psi-Disruptor build progress
	if (bShowingPsiDisruptorProgress)
	{
		// Background
		DrawRect(FColor(0, 0, 0, 200), ProgressBarX - 5, ProgressBarY - 5, 
				ProgressBarWidth + 10, ProgressBarHeight + 10);
		
		// Progress bar background
		DrawRect(FColor(50, 50, 50), ProgressBarX, ProgressBarY, ProgressBarWidth, ProgressBarHeight);
		
		// Progress bar fill
		float Progress = FMath::Clamp(PsiDisruptorBuildProgress / PsiDisruptorBuildTime, 0.0f, 1.0f);
		float FilledWidth = ProgressBarWidth * Progress;
		
		// Gradient color from yellow to red as it nears completion
		FColor BarColor = FColor::MakeRedToGreenColorFromScalar(1.0f - Progress);
		DrawRect(BarColor, ProgressBarX, ProgressBarY, FilledWidth, ProgressBarHeight);
		
		// Draw text
		float TimeRemaining = PsiDisruptorBuildTime - PsiDisruptorBuildProgress;
		FString ProgressText = FString::Printf(TEXT("PSI-DISRUPTOR BUILDING: %.1fs"), TimeRemaining);
		DrawText(ProgressText, FColor::Yellow, ProgressBarX + 10, ProgressBarY + 5, nullptr, 1.2f, false);
		
		// Draw warning
		DrawText(TEXT("DESTROY THE BUILDERS TO STOP IT!"), FColor::Orange, 
				ProgressBarX + 20, ProgressBarY + ProgressBarHeight + 10, nullptr, 1.0f, false);
		
		// Flash border when near completion
		if (Progress > 0.8f)
		{
			float Flash = FMath::Sin(GetWorld()->GetTimeSeconds() * 10.0f) * 0.5f + 0.5f;
			FColor FlashColor = FColor((uint8)(255 * Flash), 0, 0, 255);
			// Top
			DrawRect(FlashColor, ProgressBarX - 5, ProgressBarY - 5, ProgressBarWidth + 10, 3);
			// Bottom
			DrawRect(FlashColor, ProgressBarX - 5, ProgressBarY + ProgressBarHeight + 2, ProgressBarWidth + 10, 3);
			// Left
			DrawRect(FlashColor, ProgressBarX - 5, ProgressBarY - 5, 3, ProgressBarHeight + 10);
			// Right
			DrawRect(FlashColor, ProgressBarX + ProgressBarWidth + 2, ProgressBarY - 5, 3, ProgressBarHeight + 10);
		}
	}
	
	// Draw MindMeld progress (offset if both are active)
	if (bShowingMindmeldProgress)
	{
		float MindmeldY = ProgressBarY;
		if (bShowingPsiDisruptorProgress)
		{
			MindmeldY += ProgressBarHeight + 60; // Offset below Psi-Disruptor bar
		}
		
		// Background
		DrawRect(FColor(0, 0, 0, 200), ProgressBarX - 5, MindmeldY - 5, 
				ProgressBarWidth + 10, ProgressBarHeight + 10);
		
		// Progress bar background
		DrawRect(FColor(50, 50, 50), ProgressBarX, MindmeldY, ProgressBarWidth, ProgressBarHeight);
		
		// Progress bar fill
		float Progress = FMath::Clamp(MindmeldProgress / MindmeldCastTime, 0.0f, 1.0f);
		float FilledWidth = ProgressBarWidth * Progress;
		
		// Purple color for psychic ability
		FColor BarColor = FColor(128 + (uint8)(127 * Progress), 0, 255, 255);
		DrawRect(BarColor, ProgressBarX, MindmeldY, FilledWidth, ProgressBarHeight);
		
		// Draw text
		float TimeRemaining = MindmeldCastTime - MindmeldProgress;
		FString ProgressText = FString::Printf(TEXT("MINDMELD CASTING: %.1fs"), TimeRemaining);
		DrawText(ProgressText, FColor::Magenta, ProgressBarX + 10, MindmeldY + 5, nullptr, 1.2f, false);
		
		// Draw warning
		DrawText(TEXT("KILL THE MINDMELDER TO INTERRUPT!"), FColor::Red, 
				ProgressBarX + 20, MindmeldY + ProgressBarHeight + 10, nullptr, 1.0f, false);
		
		// Draw location indicator if we have a valid caster
		if (MindmeldCaster && IsValid(MindmeldCaster))
		{
			// Project caster location to screen
			FVector CasterLocation = MindmeldCaster->GetActorLocation();
			FVector2D ScreenLocation;
			
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				if (PC->ProjectWorldLocationToScreen(CasterLocation, ScreenLocation))
				{
					// Draw direction indicator
					float IndicatorSize = 30.0f;
					FColor IndicatorColor = FColor::Magenta;
					
					// Draw circle at enemy position
					DrawRect(IndicatorColor, ScreenLocation.X - IndicatorSize/2, ScreenLocation.Y - IndicatorSize/2, 
							IndicatorSize, IndicatorSize);
					
					// Draw text above the indicator
					DrawText(TEXT("MINDMELDER"), IndicatorColor, ScreenLocation.X - 40, ScreenLocation.Y - IndicatorSize - 20, 
							nullptr, 1.0f, false);
					
					// Draw distance to enemy
					if (PlayerCharacter)
					{
						float Distance = FVector::Dist(PlayerCharacter->GetActorLocation(), CasterLocation);
						FString DistanceText = FString::Printf(TEXT("%.0fm"), Distance / 100.0f);
						DrawText(DistanceText, FColor::White, ScreenLocation.X - 20, ScreenLocation.Y + IndicatorSize + 5,
								nullptr, 0.9f, false);
					}
				}
			}
		}
		
		// Critical flash when near completion
		if (Progress > 0.9f)
		{
			float Flash = FMath::Sin(GetWorld()->GetTimeSeconds() * 15.0f) * 0.5f + 0.5f;
			FColor FlashColor = FColor(255, 0, (uint8)(255 * Flash), 255);
			// Draw skull icon or death warning
			FString DeathWarning = TEXT("!!! INSTANT DEATH IMMINENT !!!");
			DrawText(DeathWarning, FlashColor, ProgressBarX + 50, MindmeldY - 25, nullptr, 1.5f, false);
		}
	}
}

// PsiDisruptor Event Handlers
void ABlackholeHUD::OnPsiDisruptorBuildStarted(const FVector& BuildLocation)
{
	bShowingPsiDisruptorProgress = true;
	PsiDisruptorBuildProgress = 0.0f;
	PsiDisruptorLocation = BuildLocation;
	
	AddNotification(TEXT("WARNING: Enemies building Psi-Disruptor!"), FColor::Orange, 5.0f);
	AddNotification(TEXT("It will disable your movement abilities!"), FColor::Yellow, 5.0f);
}

void ABlackholeHUD::OnPsiDisruptorBuildComplete()
{
	bShowingPsiDisruptorProgress = false;
	PsiDisruptorBuildProgress = 0.0f;
	
	AddNotification(TEXT("PSI-DISRUPTOR ACTIVATED!"), FColor::Red, 5.0f);
	AddNotification(TEXT("Movement abilities disabled!"), FColor::Red, 5.0f);
}

// MindMeld Event Handlers
void ABlackholeHUD::OnMindmeldStarted(float CastTime, AActor* Caster)
{
	bShowingMindmeldProgress = true;
	MindmeldProgress = 0.0f;
	MindmeldCastTime = CastTime;
	MindmeldCaster = Caster;
	
	AddNotification(TEXT("DANGER: MindMelder channeling PowerfulMindmeld!"), FColor::Magenta, 5.0f);
	AddNotification(FString::Printf(TEXT("Kill the MindMelder in %.0f seconds or your WP drops to 0!"), CastTime), FColor::Red, 5.0f);
}

void ABlackholeHUD::OnMindmeldComplete()
{
	bShowingMindmeldProgress = false;
	MindmeldProgress = 0.0f;
	MindmeldCaster = nullptr;
	
	// This shouldn't show if the player died, but just in case
	AddNotification(TEXT("MINDMELD COMPLETE - FATAL!"), FColor::Red, 3.0f);
}

void ABlackholeHUD::OnMindmeldInterrupted()
{
	bShowingMindmeldProgress = false;
	MindmeldProgress = 0.0f;
	MindmeldCaster = nullptr;
	
	AddNotification(TEXT("Mindmeld interrupted! Well done!"), FColor::Green, 3.0f);
}