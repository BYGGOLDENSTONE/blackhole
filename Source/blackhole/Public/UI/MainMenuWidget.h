#pragma once

#include "CoreMinimal.h"
#include "UI/MenuWidget.h"
#include "MainMenuWidget.generated.h"

/**
 * Main menu shown at game start
 */
UCLASS()
class BLACKHOLE_API UMainMenuWidget : public UMenuWidget
{
	GENERATED_BODY()

protected:
	virtual void SetupMenu() override;
};