#include "UI/SimplePauseMenu.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "Systems/GameStateManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Debug/CrashLogger.h"

USimplePauseMenu* USimplePauseMenu::CreateSimplePauseMenu(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return nullptr;
	}
	
	USimplePauseMenu* Widget = CreateWidget<USimplePauseMenu>(PlayerController, USimplePauseMenu::StaticClass());
	if (Widget)
	{
		Widget->BuildMenu();
	}
	return Widget;
}

void USimplePauseMenu::BuildMenu()
{
	if (!WidgetTree)
	{
		return;
	}
	
	// Create root canvas
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	
	// Create semi-transparent background
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	
	// Create menu container
	UBorder* MenuBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	MenuBorder->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
	MenuBorder->SetPadding(FMargin(40.0f));
	
	// Create vertical box for menu items
	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	
	// Add title
	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetText(FText::FromString(TEXT("PAUSED")));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 48;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleText->SetJustification(ETextJustify::Center);
	
	// Add to vertical box
	UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0, 0, 0, 40));
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	
	// Create buttons
	CreateButton(MenuBox, TEXT("RESUME"), [this]() { OnResumeClicked(); });
	CreateButton(MenuBox, TEXT("RESTART"), [this]() { OnRestartClicked(); });
	CreateButton(MenuBox, TEXT("QUIT"), [this]() { OnQuitClicked(); });
	
	// Set up hierarchy
	MenuBorder->SetContent(MenuBox);
	
	// Add to canvas
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0));
	
	UCanvasPanelSlot* MenuSlot = RootCanvas->AddChildToCanvas(MenuBorder);
	MenuSlot->SetAnchors(FAnchors(0.5f));
	MenuSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	MenuSlot->SetSize(FVector2D(400, 400));
	
	// Set as root
	WidgetTree->RootWidget = RootCanvas;
}

void USimplePauseMenu::CreateButton(UVerticalBox* Parent, const FString& Text, TFunction<void()> OnClick)
{
	if (!Parent || !WidgetTree)
	{
		return;
	}
	
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetStyle(FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f)))
		.SetHovered(FSlateColorBrush(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f)))
		.SetPressed(FSlateColorBrush(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f))));
	
	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ButtonText->SetText(FText::FromString(Text));
	FSlateFontInfo ButtonFont = ButtonText->GetFont();
	ButtonFont.Size = 24;
	ButtonText->SetFont(ButtonFont);
	ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ButtonText->SetJustification(ETextJustify::Center);
	
	Button->AddChild(ButtonText);
	
	UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
	ButtonSlot->SetPadding(FMargin(0, 10));
	ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	
	// Store callback
	ButtonCallbacks.Add(Button, OnClick);
	Button->OnClicked.AddDynamic(this, &USimplePauseMenu::OnButtonClicked);
}

void USimplePauseMenu::OnButtonClicked()
{
	// Find which button was clicked
	for (auto& Pair : ButtonCallbacks)
	{
		if (Pair.Key && Pair.Key->IsHovered())
		{
			Pair.Value();
			break;
		}
	}
}

void USimplePauseMenu::ShowMenu()
{
	if (!IsInViewport())
	{
		AddToViewport(1000);
	}
	
	SetVisibility(ESlateVisibility::Visible);
	
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
}

void USimplePauseMenu::HideMenu()
{
	SetVisibility(ESlateVisibility::Hidden);
	RemoveFromParent();
	
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
	}
}

void USimplePauseMenu::OnResumeClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameStateManager* GameStateMgr = GameInstance->GetSubsystem<UGameStateManager>())
		{
			GameStateMgr->ResumeGame();
			HideMenu();
		}
	}
}

void USimplePauseMenu::OnRestartClicked()
{
	CRASH_CHECKPOINT_MSG("OnRestartClicked Start");
	
	// Prevent multiple clicks during transition
	if (!IsValid(this) || !IsInViewport())
	{
		CRASH_LOG_ERROR("Widget invalid or not in viewport");
		return;
	}
	
	CRASH_CHECKPOINT_MSG("Getting GameInstance");
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		CRASH_CHECKPOINT_MSG("Getting GameStateManager");
		if (UGameStateManager* GameStateMgr = GameInstance->GetSubsystem<UGameStateManager>())
		{
			CRASH_CHECKPOINT_MSG("Disabling buttons");
			// Disable all buttons in the map to prevent clicks during transition
			for (auto& Pair : ButtonCallbacks)
			{
				if (UButton* Button = Pair.Key)
				{
					Button->SetIsEnabled(false);
				}
			}
			
			CRASH_CHECKPOINT_MSG("Clearing button callbacks");
			// Clear button callbacks after disabling to prevent any further input
			ButtonCallbacks.Empty();
			
			// First, ensure we're in game-only input mode to prevent UI access during transition
			CRASH_CHECKPOINT_MSG("Getting World");
			if (UWorld* World = GetWorld())
			{
				CRASH_CHECKPOINT_MSG("Getting PlayerController");
				if (APlayerController* PC = World->GetFirstPlayerController())
				{
					CRASH_CHECKPOINT_MSG("Setting input mode");
					FInputModeGameOnly InputMode;
					PC->SetInputMode(InputMode);
					PC->bShowMouseCursor = false;
					
					// Unpause the game before restart
					CRASH_CHECKPOINT_MSG("Unpausing game");
					PC->SetPause(false);
				}
			}
			
			// Mark widget for pending kill to prevent access during level transition
			CRASH_CHECKPOINT_MSG("Marking widget as garbage");
			MarkAsGarbage();
			
			// Hide widget instead of removing immediately
			CRASH_CHECKPOINT_MSG("Hiding widget");
			SetVisibility(ESlateVisibility::Collapsed);
			
			// Perform restart - GameStateManager will handle cleanup
			CRASH_CHECKPOINT_MSG("Calling RestartGame");
			GameStateMgr->RestartGame();
			
			CRASH_CHECKPOINT_MSG("OnRestartClicked End");
		}
	}
}

void USimplePauseMenu::OnQuitClicked()
{
	// Prevent multiple clicks during transition
	if (!IsValid(this) || !IsInViewport())
	{
		return;
	}
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameStateManager* GameStateMgr = GameInstance->GetSubsystem<UGameStateManager>())
		{
			// Disable all buttons in the map
			for (auto& Pair : ButtonCallbacks)
			{
				if (UButton* Button = Pair.Key)
				{
					Button->SetIsEnabled(false);
				}
			}
			
			// Clear button callbacks after disabling
			ButtonCallbacks.Empty();
			
			// Hide widget before quitting
			SetVisibility(ESlateVisibility::Collapsed);
			
			GameStateMgr->QuitGame();
		}
	}
}