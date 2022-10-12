// Copyright Epic Games, Inc. All Rights Reserved.

#include "GCD_7GameMode.h"
#include "GCD_7Character.h"
#include "UObject/ConstructorHelpers.h"

AGCD_7GameMode::AGCD_7GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
