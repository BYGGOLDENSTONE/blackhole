#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SimplePauseMenu.generated.h"

class UVerticalBox;
class UButton;

/**
 * Simplified pause menu that works directly from C++
 */
UCLASS()
class BLACKHOLE_API USimplePauseMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	// Create and initialize the menu
	static USimplePauseMenu* CreateSimplePauseMenu(APlayerController* PlayerController);
	
	// Show/hide the menu
	void ShowMenu();
	void HideMenu();

protected:
	// Build the menu structure
	void BuildMenu();
	
	// Helper to create buttons
	void CreateButton(UVerticalBox* Parent, const FString& Text, TFunction<void()> OnClick);
	
	// Button callbacks
	UFUNCTION()
	void OnButtonClicked();
	
	void OnResumeClicked();
	void OnRestartClicked();
	void OnQuitClicked();
	
private:
	// Store button callbacks
	TMap<UButton*, TFunction<void()>> ButtonCallbacks;
};