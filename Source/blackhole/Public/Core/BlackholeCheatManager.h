#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "blackhole.h"
#include "BlackholeCheatManager.generated.h"

UCLASS()
class BLACKHOLE_API UBlackholeCheatManager : public UCheatManager
{
	GENERATED_BODY()
	
public:
	// Set current WillPower
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void SetWP(float Amount);
	
	// Set current Heat
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void SetHeat(float Amount);
	
	// Set character path
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void SetPath(const FString& PathName);
	
	// Start combat mode
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void StartCombat();
	
	// End combat mode
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void EndCombat();
	
	// Force disable a random ability
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void DisableRandomAbility();
	
	// Reset all resources
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void ResetResources();
	
	// Show debug info
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void ShowDebugInfo();
	
	// Toggle debug display
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void ToggleDebugDisplay();
	
	// Force ultimate mode activation
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void ForceUltimateMode();
	
	// Cache player abilities
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void CacheAbilities();
	
	// Test combo system
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void TestCombo(const FString& ComboName);
	
	// Show combo debug info
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void ShowComboInfo();
	
	// Reset combo state
	UFUNCTION(Exec, Category = "Blackhole Cheats")
	void ResetCombo();
	
private:
	bool bShowDebugDisplay = false;
};