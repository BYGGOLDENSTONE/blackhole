#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BlackholeGameMode.generated.h"

UCLASS()
class BLACKHOLE_API ABlackholeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABlackholeGameMode();

protected:
	virtual void BeginPlay() override;
};