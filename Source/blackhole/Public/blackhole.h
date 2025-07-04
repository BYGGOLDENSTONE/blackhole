// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Character path enumeration
UENUM(BlueprintType)
enum class ECharacterPath : uint8
{
	None		UMETA(DisplayName = "None"),
	Hacker		UMETA(DisplayName = "Hacker"),
	Forge		UMETA(DisplayName = "Forge")
};
