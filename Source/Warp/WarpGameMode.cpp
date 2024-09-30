// Copyright Epic Games, Inc. All Rights Reserved.

#include "WarpGameMode.h"
#include "WarpHUD.h"
#include "WarpCharacter.h"
#include "UObject/ConstructorHelpers.h"

AWarpGameMode::AWarpGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AWarpHUD::StaticClass();
}
