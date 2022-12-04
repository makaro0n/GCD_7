// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Input/VMM_InputTrigger.h"


/*-----------------------------------------------------------------------------
	UVMM_InputTrigger implementation.
-----------------------------------------------------------------------------*/

UVMM_InputTrigger::UVMM_InputTrigger(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InputID(INDEX_NONE)
	, bUseGameplayEvent(false)
{}

void UVMM_InputTrigger::SetInputID(const int32& InInputID)
{
	InputID = InInputID;
}

bool UVMM_InputTrigger::MatchTrigger(const UVMM_InputTrigger* InOtherTrigger) const
{
	check(InOtherTrigger);

	// Fast path check condition
	if (bUseGameplayEvent != InOtherTrigger->bUseGameplayEvent)
	{
		return false;
	}

	// Fast path check tags and classes
	if (Tags.Num() != InOtherTrigger->Tags.Num() || Abilities.Num() != InOtherTrigger->Abilities.Num())
	{
		return false;
	}

	// Each every tags
	for (int32 i = 0; i < Tags.Num(); i++)
	{
		// Check other trigger tag index is valid
		if (!InOtherTrigger->Tags.IsValidIndex(i))
		{
			return false;
		}

		// Check the tag match condition
		if (InOtherTrigger->Tags[i] != InOtherTrigger->Tags[i])
		{
			return false;
		}
	}

	// Each every ability classes
	for (int32 i = 0; i < Abilities.Num(); i++)
	{
		// Check other trigger class index is valid
		if (!InOtherTrigger->Abilities.IsValidIndex(i))
		{
			return false;
		}

		// Check the class match condition
		if (InOtherTrigger->Abilities[i] != InOtherTrigger->Abilities[i])
		{
			return false;
		}
	}

	// Return the result
	return true;
}

/*-----------------------------------------------------------------------------
	UVMM_InputTriggerPressed implementation.
-----------------------------------------------------------------------------*/

ETriggerState UVMM_InputTriggerPressed::UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime)
{
	// Triggered on transition to actuated.
	return IsActuated(ModifiedValue) && !IsActuated(LastValue) ? ETriggerState::Triggered : ETriggerState::None;
}

/*-----------------------------------------------------------------------------
	UVMM_InputTriggerReleased implementation.
-----------------------------------------------------------------------------*/

ETriggerState UVMM_InputTriggerReleased::UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime)
{
	// Ongoing on hold
	if (IsActuated(ModifiedValue))
	{
		return ETriggerState::Ongoing;
	}

	// Triggered on release
	if (IsActuated(LastValue))
	{
		return ETriggerState::Triggered;
	}

	return ETriggerState::None;
}