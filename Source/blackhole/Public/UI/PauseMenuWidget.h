#pragma once

#include "CoreMinimal.h"
#include "UI/MenuWidget.h"
#include "PauseMenuWidget.generated.h"

/**
 * Pause menu shown when ESC is pressed during gameplay
 */
UCLASS()
class BLACKHOLE_API UPauseMenuWidget : public UMenuWidget
{
	GENERATED_BODY()

protected:
	virtual void SetupMenu() override;
	
	UFUNCTION()
	void OnResumeClicked();
	
	UFUNCTION()
	void OnRestartClicked();
};