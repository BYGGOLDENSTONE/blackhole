#include "UI/MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Fonts/SlateFontInfo.h"

void UMainMenuWidget::SetupMenu()
{
	// Note: In C++, we can't directly set the root widget. 
	// The widget structure should be created in Blueprint.
	// This C++ code creates the components but they won't be visible
	// unless properly set up in a Blueprint widget.
	
	// For now, just create the buttons that will be added to MenuContainer
	// The actual UI layout should be done in Blueprint
	
	// Create buttons
	UButton* PlayButton = CreateMenuButton("PLAY", "Play");
	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayClicked);
	}
	
	UButton* QuitButton = CreateMenuButton("QUIT", "Quit");
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);
	}
}