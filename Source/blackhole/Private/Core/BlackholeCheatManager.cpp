#include "Core/BlackholeCheatManager.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/ComboSystem.h"

void UBlackholeCheatManager::SetWP(float Amount)
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			// Set WP to specific amount by adding difference
			float CurrentWP = ResourceMgr->GetCurrentWillPower();
			float Difference = Amount - CurrentWP;
			
			if (Difference > 0)
			{
				ResourceMgr->AddWillPower(Difference);
			}
			else
			{
				// Consume WP to reduce it
				ResourceMgr->ConsumeWillPower(-Difference);
			}
			
			UE_LOG(LogTemp, Log, TEXT("Cheat: Set WP to %.0f"), Amount);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, 
					FString::Printf(TEXT("WP set to %.0f"), Amount));
			}
		}
	}
}

void UBlackholeCheatManager::SetHeat(float Amount)
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			// Set Heat to specific amount
			float CurrentHeat = ResourceMgr->GetCurrentHeat();
			float Difference = Amount - CurrentHeat;
			ResourceMgr->AddHeat(Difference);
			
			UE_LOG(LogTemp, Log, TEXT("Cheat: Set Heat to %.0f"), Amount);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, 
					FString::Printf(TEXT("Heat set to %.0f"), Amount));
			}
		}
	}
}

void UBlackholeCheatManager::SetPath(const FString& PathName)
{
	ABlackholePlayerCharacter* PlayerChar = Cast<ABlackholePlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	if (!PlayerChar)
	{
		return;
	}
	
	if (PathName.Equals("Hacker", ESearchCase::IgnoreCase))
	{
		PlayerChar->SetCurrentPath(ECharacterPath::Hacker);
		UE_LOG(LogTemp, Log, TEXT("Cheat: Set path to Hacker"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Path set to Hacker"));
		}
	}
	else if (PathName.Equals("Forge", ESearchCase::IgnoreCase))
	{
		PlayerChar->SetCurrentPath(ECharacterPath::Forge);
		UE_LOG(LogTemp, Log, TEXT("Cheat: Set path to Forge"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Path set to Forge"));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
				TEXT("Invalid path. Use 'Hacker' or 'Forge'"));
		}
	}
}

void UBlackholeCheatManager::StartCombat()
{
	if (UThresholdManager* ThresholdMgr = GetWorld()->GetSubsystem<UThresholdManager>())
	{
		ThresholdMgr->StartCombat();
		
		UE_LOG(LogTemp, Log, TEXT("Cheat: Combat started"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Combat Started"));
		}
	}
}

void UBlackholeCheatManager::EndCombat()
{
	if (UThresholdManager* ThresholdMgr = GetWorld()->GetSubsystem<UThresholdManager>())
	{
		ThresholdMgr->EndCombat();
		
		UE_LOG(LogTemp, Log, TEXT("Cheat: Combat ended"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Combat Ended"));
		}
	}
}

void UBlackholeCheatManager::DisableRandomAbility()
{
	// Trigger a fake threshold change to disable an ability
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			// Force WP to 50% to trigger first threshold
			SetWP(50.0f);
			
			UE_LOG(LogTemp, Log, TEXT("Cheat: Triggered ability disable"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
					TEXT("Triggered ability disable (set WP to 50%)"));
			}
		}
	}
}

void UBlackholeCheatManager::ResetResources()
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			ResourceMgr->ResetResources();
			
			UE_LOG(LogTemp, Log, TEXT("Cheat: Resources reset"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Resources Reset"));
			}
		}
	}
}

void UBlackholeCheatManager::ShowDebugInfo()
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
		{
			float WP = ResourceMgr->GetCurrentWillPower();
			float MaxWP = ResourceMgr->GetMaxWillPower();
			float Heat = ResourceMgr->GetCurrentHeat();
			float MaxHeat = ResourceMgr->GetMaxHeat();
			bool Overheated = ResourceMgr->IsOverheated();
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, 
					FString::Printf(TEXT("=== Resource Status ===")));
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, 
					FString::Printf(TEXT("WP: %.0f/%.0f (%.0f%%)"), WP, MaxWP, (WP/MaxWP)*100));
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, 
					FString::Printf(TEXT("Heat: %.0f/%.0f (%.0f%%)"), Heat, MaxHeat, (Heat/MaxHeat)*100));
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, Overheated ? FColor::Red : FColor::Green, 
					FString::Printf(TEXT("Overheat: %s"), Overheated ? TEXT("YES") : TEXT("NO")));
			}
		}
		
		if (UThresholdManager* ThresholdMgr = GetWorld()->GetSubsystem<UThresholdManager>())
		{
			int32 DisabledCount = ThresholdMgr->GetDisabledAbilityCount();
			const FSurvivorBuff& Buff = ThresholdMgr->GetCurrentBuff();
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, 
					FString::Printf(TEXT("=== Combat Status ===")));
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
					FString::Printf(TEXT("Disabled Abilities: %d"), DisabledCount));
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
					FString::Printf(TEXT("Damage Buff: %.0f%%"), Buff.DamageMultiplier * 100));
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
					FString::Printf(TEXT("Cooldown Reduction: %.0f%%"), Buff.CooldownReduction * 100));
			}
		}
	}
	
	// Show current path
	if (ABlackholePlayerCharacter* PlayerChar = Cast<ABlackholePlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, 
				FString::Printf(TEXT("=== Player Status ===")));
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, 
				FString::Printf(TEXT("Current Path: %s"), *PlayerChar->GetCurrentPathName()));
		}
	}
}

void UBlackholeCheatManager::ToggleDebugDisplay()
{
	bShowDebugDisplay = !bShowDebugDisplay;
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::White, 
			FString::Printf(TEXT("Debug Display: %s"), bShowDebugDisplay ? TEXT("ON") : TEXT("OFF")));
	}
	
	// TODO: Implement persistent debug display in HUD
}

void UBlackholeCheatManager::ForceUltimateMode()
{
	if (UThresholdManager* ThresholdMgr = GetWorld()->GetSubsystem<UThresholdManager>())
	{
		// First ensure combat is started so abilities are cached
		if (!ThresholdMgr->IsInCombat())
		{
			ThresholdMgr->StartCombat();
		}
		
		// Force activate ultimate mode
		ThresholdMgr->ActivateUltimateMode();
		
		UE_LOG(LogTemp, Log, TEXT("Cheat: Forced ultimate mode activation"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Ultimate Mode Activated!"));
		}
	}
}

void UBlackholeCheatManager::CacheAbilities()
{
	if (UThresholdManager* ThresholdMgr = GetWorld()->GetSubsystem<UThresholdManager>())
	{
		// Force cache player abilities
		ThresholdMgr->CachePlayerAbilities();
		
		UE_LOG(LogTemp, Log, TEXT("Cheat: Cached player abilities"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Player abilities cached"));
		}
	}
}

void UBlackholeCheatManager::TestCombo(const FString& ComboName)
{
	ABlackholePlayerCharacter* PlayerChar = Cast<ABlackholePlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	if (!PlayerChar || !PlayerChar->GetComboSystem())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Player or ComboSystem not found"));
		}
		return;
	}
	
	UComboSystem* ComboSystem = PlayerChar->GetComboSystem();
	
	// Test specific combos by simulating inputs
	if (ComboName.Equals("PhantomStrike", ESearchCase::IgnoreCase))
	{
		ComboSystem->RegisterInput(EComboInputType::Dash);
		ComboSystem->RegisterInput(EComboInputType::Slash);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Testing Phantom Strike combo"));
		}
	}
	else if (ComboName.Equals("AerialRave", ESearchCase::IgnoreCase))
	{
		ComboSystem->RegisterInput(EComboInputType::Jump);
		ComboSystem->RegisterInput(EComboInputType::Slash);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, TEXT("Testing Aerial Rave combo"));
		}
	}
	else if (ComboName.Equals("TempestBlade", ESearchCase::IgnoreCase))
	{
		ComboSystem->RegisterInput(EComboInputType::Jump);
		ComboSystem->RegisterInput(EComboInputType::Dash);
		ComboSystem->RegisterInput(EComboInputType::Slash);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Purple, TEXT("Testing Tempest Blade combo"));
		}
	}
	else if (ComboName.Equals("BladeDance", ESearchCase::IgnoreCase))
	{
		ComboSystem->RegisterInput(EComboInputType::Slash);
		ComboSystem->RegisterInput(EComboInputType::Slash);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Testing Blade Dance combo"));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
				TEXT("Unknown combo. Use: PhantomStrike, AerialRave, TempestBlade, or BladeDance"));
		}
	}
}

void UBlackholeCheatManager::ShowComboInfo()
{
	ABlackholePlayerCharacter* PlayerChar = Cast<ABlackholePlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	if (!PlayerChar || !PlayerChar->GetComboSystem())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Player or ComboSystem not found"));
		}
		return;
	}
	
	UComboSystem* ComboSystem = PlayerChar->GetComboSystem();
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, TEXT("=== Combo System Status ==="));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
			FString::Printf(TEXT("In Combo: %s"), ComboSystem->IsInCombo() ? TEXT("YES") : TEXT("NO")));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
			FString::Printf(TEXT("Combo Length: %d"), ComboSystem->GetCurrentComboLength()));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
			FString::Printf(TEXT("Window Open: %s"), ComboSystem->IsComboWindowOpen() ? TEXT("YES") : TEXT("NO")));
		
		if (ComboSystem->IsComboWindowOpen())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
				FString::Printf(TEXT("Window Time: %.2fs"), ComboSystem->GetComboWindowRemaining()));
		}
		
		// Show registered combos
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, TEXT(""));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, TEXT("=== Available Combos ==="));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("1. PhantomStrike: Dash + Slash"));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("2. AerialRave: Jump + Slash"));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Purple, TEXT("3. TempestBlade: Jump + Dash + Slash"));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("4. BladeDance: Slash + Slash"));
	}
}

void UBlackholeCheatManager::ResetCombo()
{
	ABlackholePlayerCharacter* PlayerChar = Cast<ABlackholePlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	if (!PlayerChar || !PlayerChar->GetComboSystem())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Player or ComboSystem not found"));
		}
		return;
	}
	
	// Force combo system to reset by clearing patterns and re-registering
	UComboSystem* ComboSystem = PlayerChar->GetComboSystem();
	ComboSystem->ClearComboPatterns();
	
	// Re-register default combos by re-registering the patterns
	// Dash + Slash = Phantom Strike
	FComboPattern PhantomStrike;
	PhantomStrike.ComboName = "PhantomStrike";
	PhantomStrike.RequiredInputs = { EComboInputType::Dash, EComboInputType::Slash };
	PhantomStrike.TimingWindows = { 0.0f, 0.5f };
	PhantomStrike.ResourceDiscount = 0.5f;
	PhantomStrike.DamageMultiplier = 1.5f;
	ComboSystem->RegisterComboPattern(PhantomStrike);
	
	// Jump + Slash = Aerial Rave
	FComboPattern AerialRave;
	AerialRave.ComboName = "AerialRave";
	AerialRave.RequiredInputs = { EComboInputType::Jump, EComboInputType::Slash };
	AerialRave.TimingWindows = { 0.0f, 0.3f };
	AerialRave.ResourceDiscount = 0.25f;
	AerialRave.DamageMultiplier = 1.25f;
	ComboSystem->RegisterComboPattern(AerialRave);
	
	// Jump + Dash + Slash = Tempest Blade
	FComboPattern TempestBlade;
	TempestBlade.ComboName = "TempestBlade";
	TempestBlade.RequiredInputs = { EComboInputType::Jump, EComboInputType::Dash, EComboInputType::Slash };
	TempestBlade.TimingWindows = { 0.0f, 0.3f, 0.3f };
	TempestBlade.ResourceDiscount = 0.4f;
	TempestBlade.DamageMultiplier = 2.0f;
	ComboSystem->RegisterComboPattern(TempestBlade);
	
	// Slash + Slash = Blade Dance
	FComboPattern BladeDance;
	BladeDance.ComboName = "BladeDance";
	BladeDance.RequiredInputs = { EComboInputType::Slash, EComboInputType::Slash };
	BladeDance.TimingWindows = { 0.0f, 0.8f };
	BladeDance.ResourceDiscount = 0.2f;
	BladeDance.DamageMultiplier = 1.0f;
	ComboSystem->RegisterComboPattern(BladeDance);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Combo system reset"));
	}
}