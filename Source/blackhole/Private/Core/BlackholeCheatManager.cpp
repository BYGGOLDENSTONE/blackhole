#include "Core/BlackholeCheatManager.h"
#include "Systems/ResourceManager.h"
#include "Systems/ThresholdManager.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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
	// Heat system removed
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
			TEXT("Heat system has been removed"));
	}
}

void UBlackholeCheatManager::SetPath(const FString& PathName)
{
	// Path switching removed - always Hacker path
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, 
			TEXT("Path system removed - Always Hacker path"));
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
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, 
					FString::Printf(TEXT("=== Resource Status ===")));
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, 
					FString::Printf(TEXT("WP: %.0f/%.0f (%.0f%%)"), WP, MaxWP, (WP/MaxWP)*100));
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
				TEXT("Current Path: Hacker"));
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
	// Old combo system has been removed
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
			TEXT("Old combo system has been removed. Combos are now handled by DashSlashCombo and JumpSlashCombo components."));
	}
	return;
}

void UBlackholeCheatManager::ShowComboInfo()
{
	// Old combo system has been removed
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
			TEXT("Old combo system has been removed. Combos are now handled by DashSlashCombo and JumpSlashCombo components."));
	}
	return;
}

void UBlackholeCheatManager::ResetCombo()
{
	// Old combo system has been removed
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
			TEXT("Old combo system has been removed. Combos are now handled by DashSlashCombo and JumpSlashCombo components."));
	}
	return;
}