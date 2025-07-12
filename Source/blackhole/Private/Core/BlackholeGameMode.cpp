#include "Core/BlackholeGameMode.h"
#include "Player/BlackholePlayerCharacter.h"
#include "UI/BlackholeHUD.h"

ABlackholeGameMode::ABlackholeGameMode()
{
	// Set default classes - these can be overridden in Blueprint
	DefaultPlayerClass = ABlackholePlayerCharacter::StaticClass();
	DefaultHUDClass = ABlackholeHUD::StaticClass();
	
	// Assign to base class properties
	DefaultPawnClass = DefaultPlayerClass;
	HUDClass = DefaultHUDClass;
}

void ABlackholeGameMode::BeginPlay()
{
	Super::BeginPlay();
}