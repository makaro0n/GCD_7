// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Player/VMM_PlayerController.h"
#include "Player/VMM_PlayerCameraManager.h"
#include "Input/VMM_InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"


AVMM_PlayerController::AVMM_PlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerCameraManagerClass = AVMM_PlayerCameraManager::StaticClass();
}

void AVMM_PlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
}

void AVMM_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

#if 1 // Need initialize ability component
	// Make sure that we are using a UEnhancedInputComponent; if not, the project is not configured correctly.
	EnhancedInputComponent = Cast<UVMM_InputComponent>(InputComponent);

	// Check the input component is valid
	if (EnhancedInputComponent == nullptr)
	{
		return;
	}

	// Get the Enhanced Input Local Player Subsystem from the Local Player related to our Player Controller.
	if (UEnhancedInputLocalPlayerSubsystem* theSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// Define the input priority
		const int32 theInputPriority = 0;

		// PawnClientRestart can run more than once in an Actor's lifetime, so start by clearing out any leftover mappings.
		theSubsystem->ClearAllMappings();

		// Initialize the input component
		EnhancedInputComponent->InitializePlayerInput(this, theSubsystem, InputMappingContext);
	}
#endif
}