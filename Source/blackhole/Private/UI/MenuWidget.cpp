#include "UI/MenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Systems/GameStateManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Fonts/SlateFontInfo.h"
#include "Blueprint/WidgetTree.h"

void UMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	// Create root panel
	UCanvasPanel* RootPanel = NewObject<UCanvasPanel>(this, UCanvasPanel::StaticClass(), TEXT("RootPanel"));
	RootPanel->SetVisibility(ESlateVisibility::Visible);
	
	// Create background for visibility
	UButton* Background = NewObject<UButton>(this);
	Background->SetStyle(FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f)))
		.SetHovered(FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f)))
		.SetPressed(FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f))));
	Background->SetIsEnabled(false);
	
	UCanvasPanelSlot* BackgroundSlot = RootPanel->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0));
	
	// Create main container
	MenuContainer = NewObject<UVerticalBox>(this, UVerticalBox::StaticClass(), TEXT("MenuContainer"));
	MenuContainer->SetVisibility(ESlateVisibility::Visible);
	
	// Add container to root panel
	UCanvasPanelSlot* ContainerSlot = RootPanel->AddChildToCanvas(MenuContainer);
	ContainerSlot->SetAnchors(FAnchors(0.5f));
	ContainerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ContainerSlot->SetSize(FVector2D(400, 500));
	ContainerSlot->SetPosition(FVector2D(0, 0));
	
	// Set root panel as the root widget  
	// In NativeOnInitialized, we set the widget directly
	WidgetTree->RootWidget = RootPanel;
	
	// Set up the specific menu
	SetupMenu();
	
	// Force rebuild
	ForceLayoutPrepass();
}

void UMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Re-setup menu if needed (for Blueprint widgets)
	if (!MenuContainer)
	{
		NativeOnInitialized();
	}
}

void UMenuWidget::ShowMenu()
{
	if (!IsInViewport())
	{
		AddToViewport(100); // High Z-order to be on top
	}
	
	SetVisibility(ESlateVisibility::Visible);
	
	// Set input mode to UI
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void UMenuWidget::HideMenu()
{
	SetVisibility(ESlateVisibility::Hidden);
	RemoveFromParent();
	
	// Restore game input
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}

UButton* UMenuWidget::CreateMenuButton(const FString& ButtonText, const FString& ActionName)
{
	// Create button
	UButton* Button = NewObject<UButton>(this);
	Button->SetStyle(FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f)))
		.SetHovered(FSlateColorBrush(FLinearColor(0.25f, 0.25f, 0.25f, 1.0f)))
		.SetPressed(FSlateColorBrush(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f))));
	
	// Create text for button
	UTextBlock* ButtonTextBlock = NewObject<UTextBlock>(this);
	ButtonTextBlock->SetText(FText::FromString(ButtonText));
	ButtonTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ButtonTextBlock->SetJustification(ETextJustify::Center);
	
	// Use setter for font size
	FSlateFontInfo FontInfo = ButtonTextBlock->GetFont();
	FontInfo.Size = 24;
	ButtonTextBlock->SetFont(FontInfo);
	
	// Add text to button
	Button->AddChild(ButtonTextBlock);
	
	// Add button to container
	if (MenuContainer)
	{
		UVerticalBoxSlot* VBoxSlot = MenuContainer->AddChildToVerticalBox(Button);
		VBoxSlot->SetPadding(FMargin(20, 5));
		VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	
	return Button;
}

void UMenuWidget::OnPlayClicked()
{
	if (UGameStateManager* GameStateMgr = GetGameStateManager())
	{
		GameStateMgr->StartGame();
		HideMenu();
	}
}

void UMenuWidget::OnQuitClicked()
{
	if (UGameStateManager* GameStateMgr = GetGameStateManager())
	{
		GameStateMgr->QuitGame();
	}
}

void UMenuWidget::OnMainMenuClicked()
{
	if (UGameStateManager* GameStateMgr = GetGameStateManager())
	{
		GameStateMgr->EndGame(false);
		GameStateMgr->ResetGame();
		// TODO: Load main menu level
	}
}

UGameStateManager* UMenuWidget::GetGameStateManager() const
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this))
	{
		return GameInstance->GetSubsystem<UGameStateManager>();
	}
	return nullptr;
}