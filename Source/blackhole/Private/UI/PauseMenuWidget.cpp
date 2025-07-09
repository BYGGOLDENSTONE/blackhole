#include "UI/PauseMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Systems/GameStateManager.h"
#include "Fonts/SlateFontInfo.h"

void UPauseMenuWidget::SetupMenu()
{
	if (!MenuContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("PauseMenuWidget: MenuContainer is null!"));
		return;
	}
	
	// Add title
	UTextBlock* TitleText = NewObject<UTextBlock>(this);
	TitleText->SetText(FText::FromString("PAUSED"));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	
	// Use setter for font size
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 36;
	TitleText->SetFont(TitleFont);
	
	TitleText->SetJustification(ETextJustify::Center);
	
	// Add title to menu container
	UVerticalBoxSlot* TitleSlot = MenuContainer->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0, 0, 0, 30));
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	
	// Create buttons
	UButton* ResumeButton = CreateMenuButton("RESUME", "Resume");
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	}
	
	UButton* RestartButton = CreateMenuButton("RESTART", "Restart");
	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnRestartClicked);
	}
	
	UButton* MainMenuButton = CreateMenuButton("MAIN MENU", "MainMenu");
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnMainMenuClicked);
	}
	
	UButton* QuitButton = CreateMenuButton("QUIT", "Quit");
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnQuitClicked);
	}
}

void UPauseMenuWidget::OnResumeClicked()
{
	if (UGameStateManager* GameStateMgr = GetGameStateManager())
	{
		GameStateMgr->ResumeGame();
		HideMenu();
	}
}

void UPauseMenuWidget::OnRestartClicked()
{
	if (UGameStateManager* GameStateMgr = GetGameStateManager())
	{
		HideMenu();
		GameStateMgr->ResetGame();
		GameStateMgr->StartGame();
	}
}