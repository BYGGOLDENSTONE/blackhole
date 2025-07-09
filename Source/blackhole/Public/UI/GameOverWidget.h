#pragma once

#include "CoreMinimal.h"
#include "UI/MenuWidget.h"
#include "GameOverWidget.generated.h"

/**
 * Game over menu shown when player dies
 */
UCLASS()
class BLACKHOLE_API UGameOverWidget : public UMenuWidget
{
	GENERATED_BODY()

protected:
	virtual void SetupMenu() override;
	
	UFUNCTION()
	void OnPlayAgainClicked();
};