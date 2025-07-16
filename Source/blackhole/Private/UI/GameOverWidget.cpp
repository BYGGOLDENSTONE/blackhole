#include "UI/GameOverWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Systems/GameStateManager.h"
#include "Fonts/SlateFontInfo.h"

void UGameOverWidget::SetupMenu()
{
	// Create main panel
	UCanvasPanel* RootPanel = NewObject<UCanvasPanel>(this);
	
	// Add title
	UTextBlock* TitleText = NewObject<UTextBlock>(this); 
	TitleText->SetText(FText::FromString("YOU DIED"));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
	
	// Use setter for font size
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 48;
	TitleText->SetFont(TitleFont);
	
	TitleText->SetJustification(ETextJustify::Center);
	
	// Add subtitle
	UTextBlock* SubtitleText = NewObject<UTextBlock>(this);
	SubtitleText->SetText(FText::FromString("The void consumed you..."));
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
	
	// Use setter for font size
	FSlateFontInfo SubtitleFont = SubtitleText->GetFont();
	SubtitleFont.Size = 18;
	SubtitleText->SetFont(SubtitleFont);
	
	SubtitleText->SetJustification(ETextJustify::Center);
	
	// Center the menu container
	if (MenuContainer)
	{
		UCanvasPanelSlot* ContainerSlot = RootPanel->AddChildToCanvas(MenuContainer);
		ContainerSlot->SetAnchors(FAnchors(0.5f));
		ContainerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ContainerSlot->SetSize(FVector2D(300, 400));
		ContainerSlot->SetPosition(FVector2D(0, 50));
	}
	
	// Add title to canvas
	UCanvasPanelSlot* TitleSlot = RootPanel->AddChildToCanvas(TitleText);
	TitleSlot->SetAnchors(FAnchors(0.5f, 0.3f));
	TitleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	TitleSlot->SetSize(FVector2D(400, 100));
	
	// Add subtitle to canvas
	UCanvasPanelSlot* SubtitleSlot = RootPanel->AddChildToCanvas(SubtitleText);
	SubtitleSlot->SetAnchors(FAnchors(0.5f, 0.35f));
	SubtitleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	SubtitleSlot->SetSize(FVector2D(400, 50));
	
	// Create buttons
	UButton* PlayAgainButton = CreateMenuButton("PLAY AGAIN", "PlayAgain");
	if (PlayAgainButton)
	{
		PlayAgainButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnPlayAgainClicked);
	}
	
	UButton* MainMenuButton = CreateMenuButton("MAIN MENU", "MainMenu");
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnMainMenuClicked);
	}
	
	UButton* QuitButton = CreateMenuButton("QUIT", "Quit");
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnQuitClicked);
	}
}

void UGameOverWidget::OnPlayAgainClicked()
{
	if (UGameStateManager* GameStateMgr = GetGameStateManager())
	{
		HideMenu();
		GameStateMgr->ResetGame();
		GameStateMgr->StartGame();
	}
}