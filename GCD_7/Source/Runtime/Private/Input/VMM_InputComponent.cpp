// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Input/VMM_InputComponent.h"
#include "Input/VMM_InputTrigger.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Librarys/VMM_Library.h"
#include "Player/VMM_PlayerCameraManager.h"
#include "Input/VMM_InputAction.h"
#include "TimerManager.h"

UVMM_InputComponent::UVMM_InputComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ViewInputRate(FVector2D(1.f))
	, InputDirection(EInputDirection::MAX)
	, LastInputDirection(EInputDirection::MAX)
{}

#pragma region Initialize
void UVMM_InputComponent::InitializePlayerInput(APlayerController* InPlayerController, UEnhancedInputLocalPlayerSubsystem* InSubsSystem, const UVMM_InputMappingContext* InMappingContext)
{
	// Cache the player controller reference
	check(InPlayerController);
	PlayerController = InPlayerController;

	// Initialize the input mapping context
 	if (const UVMM_InputMappingContext* theInputMappingContext = InMappingContext)
 	{
		// Define the input id
		int32 theInputID = INDEX_NONE;

 		// Define the input priority
 		const int32 theInputPriority = 0;
 
		// Add each mapping context, along with their priority values. Higher values out prioritize lower values.
		InSubsSystem->AddMappingContext(theInputMappingContext, theInputPriority);

		// Bind each look input
		ViewInputRate = theInputMappingContext->ViewInputRate;
		const TArray<FInputDefinition>& theLookInputDefinitions = theInputMappingContext->LookInputDefinitions;
		if (theLookInputDefinitions.Num() >= 4)
		{
			BindAction(theLookInputDefinitions[0].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_LookPitch);
			BindAction(theLookInputDefinitions[1].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_LookYaw);
			//BindAction(theLookInputDefinitions[2].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_LookRoll);
			BindAction(theLookInputDefinitions[3].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_LookDistance);
		}

		// Bind each movement input
		const TArray<FInputDefinition>& theMovementInputDefinitions = theInputMappingContext->MovementInputDefinitions;
		if(theMovementInputDefinitions.Num() >= 5)
		{
			BindAction(theMovementInputDefinitions[0].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
			BindAction(theMovementInputDefinitions[0].Action, ETriggerEvent::Completed, this, &ThisClass::Input_StopMove);
			BindAction(theMovementInputDefinitions[1].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_MoveFoward);
			BindAction(theMovementInputDefinitions[2].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_MoveBackward);
			BindAction(theMovementInputDefinitions[3].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_MoveLeft);
			BindAction(theMovementInputDefinitions[4].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_MoveRight);
		}

		// Bind each movement ability input
		const TArray<FInputDefinition>& theMovementAbilityInputDefinitions = theInputMappingContext->MovementAbilityInputDefinitions;
		if (theMovementAbilityInputDefinitions.Num() > 0)
		{
			BindAction(theMovementAbilityInputDefinitions[0].Action, ETriggerEvent::Triggered, this, &ThisClass::Input_MoveAbility);
		}

		// Bind each ability input
		for (const FInputDefinition& theInputDefinition : theInputMappingContext->AbilityInputDefinitions)
		{
			// Check the input action is valid
			if (theInputDefinition.Action == nullptr)
			{
				continue;
			}

			// Check the bind input id condition
			if (theInputDefinition.Action->BindInputID())
			{
				AbilityInputActions.Add(theInputDefinition.Action);
			}

			// Bind default action
			BindAction(theInputDefinition.Action, ETriggerEvent::Triggered, this, &ThisClass::ResponseAbilityEvent);
		}
 	}
}
#pragma endregion


#pragma region Ability
void UVMM_InputComponent::TryActivateAbilities(AActor* InActor, const FInputActionInstance& InInputActionInstance, const UVMM_InputTrigger* InInputTrigger)
{
	check(InActor && InInputTrigger);

	// Check the input trigger is valid
	if (InInputTrigger == nullptr)
	{
		return;
	}

	// Check the ability component is valid
	UAbilitySystemComponent* theAbilityComp = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor);
	if (theAbilityComp == nullptr)
	{
		return;
	}

	// Try activate ability input id
	// Now we are only compatible with one mode, and we may be compatible with two modes in the future
	if (TryActivateAbilitiesInputID(theAbilityComp, InInputActionInstance, InInputTrigger))
	{
		return;
	}

	// Try active abilities tag
	if (InInputTrigger->HasTags())
	{
		for (const FGameplayTag& theGameplayTag : InInputTrigger->Tags)
		{
			// Check the class is valid
			if (!theGameplayTag.IsValid())
			{
				continue;
			}

			// Try activate abilities
			if (InInputTrigger->UseGameplayEvent())
			{
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InActor, theGameplayTag, FGameplayEventData());
			}
			else
			{
				theAbilityComp->TryActivateAbilitiesByTag(FGameplayTagContainer(theGameplayTag));
			}
		}
	}

	// Try active abilities class
	if (InInputTrigger->HasAbilities())
	{
		for (const TSubclassOf<UGameplayAbility>& theAbilityClass : InInputTrigger->Abilities)
		{
			// Check the class is valid
			if (theAbilityClass == NULL)
			{
				continue;
			}

			// Try activate abilities
			theAbilityComp->TryActivateAbilityByClass(theAbilityClass);
		}
	}
}

bool UVMM_InputComponent::TryActivateAbilitiesInputID(UAbilitySystemComponent* InAbilityComponent, const FInputActionInstance& InInputActionInstance, const UVMM_InputTrigger* InInputTrigger)
{
	check(InAbilityComponent && InInputTrigger);

	// Define the response condition
	bool bResponseInput = false;

#if 0 // Need modify engine plugin
	// Check the input action is valid
	const UVMM_InputAction* theInputAction = Cast<const UVMM_InputAction>(InInputActionInstance.GetInputAction());
	if (theInputAction == nullptr)
	{
		return;
	}

	// Check the bind input id condition
	if (!theInputAction->NeedBindInputID())
	{
		return;
	}

	// Find the cached input id
	for (int32 theInputID = 0; theInputID < AbilityInputActions.Num(); theInputID++)
	{
#if 0 // Need modify engine plugin
		if (AbilityInputActions[theInputID] != theInputAction)
		{
			continue;
		}

		// Choose the trigger event
		if (InInputTrigger->IsA(UVMM_InputTriggerPressed::StaticClass()))
		{
			InAbilityComponent->AbilityLocalInputPressed(theInputID);
		}
		else if (InInputTrigger->IsA(UVMM_InputTriggerReleased::StaticClass()))
		{
			InAbilityComponent->AbilityLocalInputReleased(theInputID);
		}
#else
		// Each every source trigger
		for (const UInputTrigger* theSourceTrigger : AbilityInputActions[theInputID]->Triggers)
		{
			// Check the source trigger is valid
			const UVMM_InputTrigger* theVirtualSourceTrigger = Cast<const UVMM_InputTrigger>(theSourceTrigger);
			if (theVirtualSourceTrigger == nullptr)
			{
				continue;
			}

			// Check the input id is match
			if (theInputID != theVirtualSourceTrigger->GetInputID())
			{
				// Global input id now.
				break;
			}

			// Choose the trigger event
			if (InInputTrigger->IsA(UVMM_InputTriggerPressed::StaticClass()) && theVirtualSourceTrigger->IsA(UVMM_InputTriggerPressed::StaticClass()))
			{
				if (true || InInputTrigger->MatchTrigger(theVirtualSourceTrigger))
				{
					InAbilityComponent->AbilityLocalInputPressed(theInputID);
				}
			}
			else if (InInputTrigger->IsA(UVMM_InputTriggerReleased::StaticClass()) && theVirtualSourceTrigger->IsA(UVMM_InputTriggerReleased::StaticClass()))
			{
				if (true || InInputTrigger->MatchTrigger(theVirtualSourceTrigger))
				{
					InAbilityComponent->AbilityLocalInputReleased(theInputID);
				}
			}
		}
#endif
		// Finished
		return;
	}
#else
	// Check the input id is valid
	if (InInputTrigger->HasInputID() == false)
	{
		return bResponseInput;
	}

	// Check the cached input action is valid
	const int32& theInputID = InInputTrigger->GetInputID();
	const UVMM_InputAction* theAbilityInputActions = AbilityInputActions.IsValidIndex(theInputID) ? AbilityInputActions[theInputID] : nullptr;
	if (theAbilityInputActions == nullptr)
	{
		return bResponseInput;
	}

	// Each every source trigger
	for (const UInputTrigger* theSourceTrigger : theAbilityInputActions->Triggers)
	{
		// Check the source trigger is valid
		const UVMM_InputTrigger* theVirtualSourceTrigger = Cast<const UVMM_InputTrigger>(theSourceTrigger);
		if (theVirtualSourceTrigger == nullptr)
		{
			continue;
		}

		// Check the input id is match
		if (theInputID != theVirtualSourceTrigger->GetInputID())
		{
			// Global input id now.
			break;
		}

		// Choose the trigger event
		if (InInputTrigger->IsA(UVMM_InputTriggerPressed::StaticClass()) && theVirtualSourceTrigger->IsA(UVMM_InputTriggerPressed::StaticClass()))
		{
			if (true || InInputTrigger->MatchTrigger(theVirtualSourceTrigger))
			{
				bResponseInput = true;
				InAbilityComponent->AbilityLocalInputPressed(theInputID);
			}
		}
		else if (InInputTrigger->IsA(UVMM_InputTriggerReleased::StaticClass()) && theVirtualSourceTrigger->IsA(UVMM_InputTriggerReleased::StaticClass()))
		{
			if (true || InInputTrigger->MatchTrigger(theVirtualSourceTrigger))
			{
				bResponseInput = true;
				InAbilityComponent->AbilityLocalInputReleased(theInputID);
			}
		}
	}
#endif

	// Return the result
	return bResponseInput;
}

void UVMM_InputComponent::ResponseAbilityEvent(const FInputActionInstance& InInputActionInstance)
{
	// Check the player controller is valid
	if (PlayerController == nullptr)
	{
		return;
	}

	// Find current response input trigger
	const UVMM_InputTrigger* theVirtualInputTrigger = GetInputTrigger(InInputActionInstance);
	if (theVirtualInputTrigger == nullptr)
	{
		return;
	}

	// Try active the trigger abilities
	TryActivateAbilities(PlayerController->GetPawn(), InInputActionInstance, theVirtualInputTrigger);
}
#pragma endregion


#pragma region Input Trigger
const UVMM_InputTrigger* UVMM_InputComponent::GetInputTrigger(const FInputActionInstance& InInputActionInstance)
{
	// Get the input trigger data
	int32 theInputTriggerIndex = INDEX_NONE;
	const TArray<UInputTrigger*>& theInputTriggers = InInputActionInstance.GetTriggers();

	// Only response trigger event now.
	if (InInputActionInstance.GetTriggerEvent() == ETriggerEvent::Triggered)
	{
		// Get the response value
		const bool bResponse = InInputActionInstance.GetValue().Get<bool>();

		// Find current trigger index
		for (int32 TriggerIndex = 0; TriggerIndex < theInputTriggers.Num(); TriggerIndex++)
		{
			// Check the input trigger is valid
			const UInputTrigger* theInputTrigger = theInputTriggers[TriggerIndex];
			if (theInputTrigger == nullptr)
			{
				continue;
			}

			// Choose the trigger event
			if (bResponse && theInputTrigger->IsA(UVMM_InputTriggerPressed::StaticClass()))
			{
				theInputTriggerIndex = TriggerIndex;
				break;
			}
			else if (!bResponse && theInputTrigger->IsA(UVMM_InputTriggerReleased::StaticClass()))
			{
				theInputTriggerIndex = TriggerIndex;
				break;
			}
		}
	}

	// Check the current input trigger is valid
	if (!theInputTriggers.IsValidIndex(theInputTriggerIndex))
	{
		return nullptr;
	}

	// Find current response input trigger
	return static_cast<const UVMM_InputTrigger*>(theInputTriggers[theInputTriggerIndex]);
}
#pragma endregion


#pragma region Look
void UVMM_InputComponent::Input_LookMode(const FInputActionValue& InInputActionValue)
{
	// Check the player controller is valid
	if (PlayerController == nullptr)
	{
		return;
	}

}

void UVMM_InputComponent::Input_LookMouse(const FInputActionValue& InInputActionValue)
{
	// Check the player controller is valid
	if (PlayerController == nullptr)
	{
		return;
	}

	// Get the input value
	const FVector2D theValue = InInputActionValue.Get<FVector2D>();

	// Response look yaw event
	ResponseLookYaw(theValue.X);

	// Response look pitch event
	ResponseLookPitch(theValue.Y * -1.f);
}

void UVMM_InputComponent::Input_LookStick(const FInputActionValue& InInputActionValue)
{

}

void UVMM_InputComponent::Input_LookYaw(const FInputActionValue& InInputActionValue)
{
	// Check the player controller is valid
	if (PlayerController == nullptr)
	{
		return;
	}

	// Get the input value
	const float& theValue = InInputActionValue.Get<float>();

	// Response look yaw event
	ResponseLookYaw(theValue);
}

void UVMM_InputComponent::Input_LookPitch(const FInputActionValue& InInputActionValue)
{
	// Check the player controller is valid
	if (PlayerController == nullptr)
	{
		return;
	}

	// Get the input value
	const float& theValue = InInputActionValue.Get<float>();

	// Response look yaw event
	ResponseLookPitch(theValue);
}

void UVMM_InputComponent::Input_LookDistance(const FInputActionValue& InInputActionValue)
{
	// Check the player controller is valid
	if (PlayerController == nullptr)
	{
		return;
	}

	// Notify player camera manager
	if (AVMM_PlayerCameraManager* theCameraManager = Cast<AVMM_PlayerCameraManager>(PlayerController->PlayerCameraManager))
	{
		// Get the input value
		const float theValue = InInputActionValue.Get<float>();

		// Response
		theCameraManager->SetCameraArmLengthScale(theValue);
	}
}

void UVMM_InputComponent::ResponseLookYaw(float InValue)
{
	// Check the value is valid
	if (InValue == 0.f)
	{
		return;
	}

	// Check the player controller is valid
	if (PlayerController == nullptr)
	{
		return;
	}

	// Check the agent perform rotation condition
	if (PlayerController->IsLookInputIgnored())
	{
		return;
	}

	// Additive view input rate
	InValue *= ViewInputRate.X;

	// Add yaw input
	PlayerController->AddYawInput(InValue);
}

void UVMM_InputComponent::ResponseLookPitch(float InValue)
{
	// Check the value is valid
	if (InValue == 0.f)
	{
		return;
	}

	// Check the player controller is valid
	if (PlayerController == nullptr)
	{
		return;
	}

	// Check the agent perform rotation condition
	if (PlayerController->IsLookInputIgnored())
	{
		return;
	}

	// Additive view input rate
	InValue *= ViewInputRate.Y;

	// Add yaw input
	PlayerController->AddPitchInput(InValue);
}
#pragma endregion


#pragma region Movement
void UVMM_InputComponent::SetMoveAxisValue(const bool& InForwardAixs, const float& InAxisValue)
{
	if (InForwardAixs)
	{
		if (MoveAxis.X != InAxisValue)
		{
			MoveAxis.X = InAxisValue;
		}
	}
	else
	{
		if (MoveAxis.Y != InAxisValue)
		{
			MoveAxis.Y = InAxisValue;
		}
	}
}

bool UVMM_InputComponent::SetInputDirection()
{
	// Calculate the desired input result
	FRotator theDesiredInputRotation = InputRotation;
	EInputDirection theDesiredInputDirection = GetPlayerInputDirection(&theDesiredInputRotation);

	// Check the input direction is changed
	if (InputDirection != theDesiredInputDirection)
	{
		LastInputDirection = InputDirection;
		InputDirection = theDesiredInputDirection;

		LastInputRotation = InputRotation;
		InputRotation = theDesiredInputRotation;

		// Only cache valid input rotation
		if (InputDirection != EInputDirection::MAX)
		{
			CachedValidInputRotation = InputRotation;
		}

		// Success
		return true;
	}

	// Return the result
	return InputDirection == EInputDirection::MAX;
}

EInputDirection UVMM_InputComponent::GetPlayerInputDirection(FRotator* OutRotation)
{
	// Define the desired input result
	FRotator theDesiredInputRotation = InputRotation;
	EInputDirection theDesiredInputDirection = EInputDirection::MAX;

	// Check the move axis is valid
	if (IsValidMoveAxis())
	{
		// Calculate the input angle value
		theDesiredInputRotation.Yaw = FMath::RadiansToDegrees(FMath::Atan2(MoveAxis.Y, MoveAxis.X));

		// Convert the angle to input direction type
		theDesiredInputDirection = UVMM_Library::GetInputDirectionType(theDesiredInputRotation.Yaw);
	}
	else
	{
		theDesiredInputRotation.Yaw = 0.f;
	}

	// Output the other result
	if (OutRotation != nullptr)
	{
		*OutRotation = theDesiredInputRotation;
	}

	// Return the result
	return theDesiredInputDirection;
}

float UVMM_InputComponent::GetPlayerInputAngle()
{
	// Calculate current player input data
	FRotator theInputRotation;
	GetPlayerInputDirection(&theInputRotation);

	// Return the result
	return theInputRotation.Yaw;
}

void UVMM_InputComponent::Input_Move(const FInputActionInstance& InInputActionInstance)
{
	// Check the move axis is valid
	const FVector2D theMoveAxis = InInputActionInstance.GetValue().Get<FVector2D>();

#if 0 // Response completed trigger
	// If the value is very small, we should stop input???
	if (theMoveAxis.IsNearlyZero(0.05f) && !HasPressedInput())
	{
		MoveAxis = FVector2D::ZeroVector;
		OnResponseCachedInputDirection();
		return;
	}
#endif

	// Calculate final input direction
	MoveAxis.X = theMoveAxis.Y;
	MoveAxis.Y = theMoveAxis.X;
	OnResponseCachedInputDirection();
}

void UVMM_InputComponent::Input_StopMove(const FInputActionInstance& InInputActionInstance)
{
	// If has other pressed input, we don't execute it.
	if (HasPressedInput() == true)
	{
		return;
	}

	// Stop input
	MoveAxis = FVector2D::ZeroVector;
	OnResponseCachedInputDirection();
}

void UVMM_InputComponent::Input_MoveFoward(const FInputActionInstance& InInputActionInstance)
{
	ResponseMovementInput(InInputActionInstance, EInputDirection::Forward);
}

void UVMM_InputComponent::Input_MoveBackward(const FInputActionInstance& InInputActionInstance)
{
	ResponseMovementInput(InInputActionInstance, EInputDirection::Backward);
}

void UVMM_InputComponent::Input_MoveLeft(const FInputActionInstance& InInputActionInstance)
{
	ResponseMovementInput(InInputActionInstance, EInputDirection::Left);
}

void UVMM_InputComponent::Input_MoveRight(const FInputActionInstance& InInputActionInstance)
{
	ResponseMovementInput(InInputActionInstance, EInputDirection::Right);
}

void UVMM_InputComponent::Input_MoveAbility(const FInputActionInstance& InInputActionInstance)
{
	// Define the pressed state
	const bool bPressed = InInputActionInstance.GetValue().Get<bool>();

	// Call input direction delegate
	if (MovementAbilityInputDelegate.IsBound())
	{
		MovementAbilityInputDelegate.Broadcast(bPressed);
	}
}

void UVMM_InputComponent::ResponseMovementInput(const FInputActionInstance& InInputActionInstance, EInputDirection InInputDirectionType)
{
	// Define the pressed state
	const bool bPressed = InInputActionInstance.GetValue().Get<bool>();

	// Response the pressed event
	if (bPressed)
	{
		// Define response delay input condition
		bool bResponseDelayInputDirection = false;

#if EnableDelayResponsePlayerInput
		// If there is no input, we record the time of the first input
		bResponseDelayInputDirection = !HasPressedInput();
#endif

		// Response input direction
		switch (InInputDirectionType)
		{
		case EInputDirection::Forward:
			bPressedForward = true;
			SetMoveAxisValue(true, 1.f);
			break;

		case EInputDirection::Backward:
			bPressedBackward = true;
			SetMoveAxisValue(true, -1.f);
			break;

		case EInputDirection::Left:
			bPressedLeft = true;
			SetMoveAxisValue(false, -1.f);
			break;

		case EInputDirection::Right:
			bPressedRight = true;
			SetMoveAxisValue(false, 1.f);
			break;
		}
		
#if EnableDelayResponsePlayerInput
		// Check delay response condition
		if (bResponseDelayInputDirection && PlayerController)
		{
			const float theDeltaTime = UGameplayStatics::GetWorldDeltaSeconds(this);
			const float theDelayTime = FMath::Min(FMath::Max(0.05f, theDeltaTime), 0.11f);
			PlayerController->GetWorldTimerManager().SetTimer(DelayResponseInputTimer, this, &UVMM_InputComponent::OnResponseCachedInputDirection, theDelayTime, false, theDelayTime);

			// If input is being cached, we do not immediately respond to input events
			return;
		}
#endif
	}
	else
	{
		switch (InInputDirectionType)
		{
		case EInputDirection::Forward:
			bPressedForward = false;
			SetMoveAxisValue(true, bPressedBackward ? -1.f : 0.f);
			break;

		case EInputDirection::Backward:
			bPressedBackward = false;
			SetMoveAxisValue(true, bPressedForward ? 1.f : 0.f);
			break;

		case EInputDirection::Left:
			bPressedLeft = false;
			SetMoveAxisValue(false, bPressedRight ? 1.f : 0.f);
			break;

		case EInputDirection::Right:
			bPressedRight = false;
			SetMoveAxisValue(false, bPressedLeft ? -1.f : 0.f);
			break;
		}
	}

	// Calculate final input direction
	OnResponseCachedInputDirection();
}

void UVMM_InputComponent::OnResponseCachedInputDirection()
{
	// Response set new input direction
	const bool bHasInputChanged = SetInputDirection();
	if (bHasInputChanged == false)
	{
		return;
	}

	// Clear cached delay response input timer
	if (DelayResponseInputTimer.IsValid() && PlayerController)
	{
		PlayerController->GetWorldTimerManager().ClearTimer(DelayResponseInputTimer);
	}

	// Call input direction delegate
	if (InputDirectionDelegate.IsBound())
	{
		InputDirectionDelegate.Broadcast(LastInputDirection, InputDirection, LastInputRotation, InputRotation);
	}
}
#pragma endregion