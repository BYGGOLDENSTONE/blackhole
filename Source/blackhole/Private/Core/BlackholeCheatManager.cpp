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