#include "Core/BlackholeGameMode.h"
#include "Player/BlackholePlayerCharacter.h"
#include "UI/BlackholeHUD.h"

ABlackholeGameMode::ABlackholeGameMode()
{
	DefaultPawnClass = ABlackholePlayerCharacter::StaticClass();
	HUDClass = ABlackholeHUD::StaticClass();
}

void ABlackholeGameMode::BeginPlay()
{
	Super::BeginPlay();
}