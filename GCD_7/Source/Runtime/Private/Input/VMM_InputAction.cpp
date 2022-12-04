// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Input/VMM_InputAction.h"
#include "Input/VMM_InputTrigger.h"

UVMM_InputAction::UVMM_InputAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InputID(INDEX_NONE)
	, bBindInputID(false)
{
	KeysDefinition.AddDefaulted();
}


#if WITH_EDITOR
void UVMM_InputAction::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(InPropertyChangedEvent);

	// Check the property is valid
	if (!InPropertyChangedEvent.Property)
	{
		return;
	}

	// Collect all virtual input action create unique id
	uint16 theCollectNumber = 0;

	// Get the property name
	const FName thePropertyName = InPropertyChangedEvent.Property->GetFName();

	// Handle event
	if (thePropertyName == GET_MEMBER_NAME_CHECKED(UVMM_InputAction, bBindInputID)
		|| thePropertyName == GET_MEMBER_NAME_CHECKED(UVMM_InputAction, Triggers))
	{
		// Each all virtual input action data asset reference
		for (TObjectIterator<UVMM_InputAction> InputActionIterator; InputActionIterator; ++InputActionIterator)
		{
			// Check the data asset is valid
			UVMM_InputAction* theInputAction = *InputActionIterator;
			if (theInputAction && theInputAction->BindInputID())
			{
				theInputAction->SetInputID(theCollectNumber);
				theInputAction->PostEditChange();
				theInputAction->MarkPackageDirty();
				++theCollectNumber;
			}
			else
			{
				theInputAction->SetInputID(INDEX_NONE);
				theInputAction->PostEditChange();
				theInputAction->MarkPackageDirty();
			}
		}
	}
}
#endif // WITH_EDITOR

void UVMM_InputAction::SetInputID(const int32& InInputID)
{
	InputID = InInputID;

	// Each every input trigger
	for (UInputTrigger* theInputTrigger : Triggers)
	{
		if (UVMM_InputTrigger* theVirtualInputTrigger = Cast<UVMM_InputTrigger>(theInputTrigger))
		{
			theVirtualInputTrigger->SetInputID(InInputID);
		}
	}
}