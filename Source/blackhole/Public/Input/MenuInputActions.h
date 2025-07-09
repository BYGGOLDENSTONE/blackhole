#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "MenuInputActions.generated.h"

/**
 * Input actions for menu system
 */
UCLASS()
class BLACKHOLE_API UMenuInputActions : public UObject
{
	GENERATED_BODY()

public:
	// Action for opening/closing menus (Quote key)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* MenuToggleAction;
};