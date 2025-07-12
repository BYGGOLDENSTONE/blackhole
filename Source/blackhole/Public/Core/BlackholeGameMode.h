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
	
	// Default classes for spawning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Mode")
	TSubclassOf<class ABlackholePlayerCharacter> DefaultPlayerClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Mode")
	TSubclassOf<class ABlackholeHUD> DefaultHUDClass;
};