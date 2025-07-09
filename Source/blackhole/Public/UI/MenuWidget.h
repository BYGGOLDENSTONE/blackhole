#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

/**
 * Base menu widget for all game menus
 */
UCLASS(Abstract)
class BLACKHOLE_API UMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeOnInitialized() override;
	
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowMenu();
	
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void HideMenu();

protected:
	// Helper to create buttons
	UFUNCTION()
	UButton* CreateMenuButton(const FString& ButtonText, const FString& ActionName);
	
	// Override in derived classes to set up menu
	virtual void SetupMenu() {}
	
	// Common button handlers
	UFUNCTION()
	virtual void OnPlayClicked();
	
	UFUNCTION()
	virtual void OnQuitClicked();
	
	UFUNCTION()
	virtual void OnMainMenuClicked();
	
	// Root panel for menu items
	UPROPERTY()
	UVerticalBox* MenuContainer;
	
	// Get game state manager
	class UGameStateManager* GetGameStateManager() const;
};