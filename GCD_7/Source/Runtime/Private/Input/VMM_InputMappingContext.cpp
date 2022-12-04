// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Input/VMM_InputMappingContext.h"
#include "Input/VMM_InputAction.h"
#include "Input/VMM_InputTrigger.h"

#include "VirtualMotionMatching.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsSettings.h"


UVMM_InputMappingContext::UVMM_InputMappingContext(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ViewInputRate(FVector2D(0.65f, 1.f))
	, ViewYawLimitAngle(FVector2D(-180.f, 180.f))
	, ViewPitchLimitAngle(FVector2D(-90.f, 90.f))
{
	LookInputDefinitions.SetNum(4);
	MovementInputDefinitions.SetNum(5);

	// Each every input direction type
	for (int32 DirectionIndex = 0; DirectionIndex < int32(EInputDirection::MAX); DirectionIndex++)
	{
		// Convert the int to enum
		const EInputDirection theInputDirection = EInputDirection(DirectionIndex);

		// Add the direction value
		FVector2D& theRange = InputDirectionRangeMap.FindOrAdd(theInputDirection);

		// Set the default angle range
		switch (theInputDirection)
		{
		case EInputDirection::Forward:
			theRange = FVector2D(-15.f, 15.f);
			break;
		case EInputDirection::ForwardLeft:
			theRange = FVector2D(-75.f, -15.f);
			break;
		case EInputDirection::ForwardRight:
			theRange = FVector2D(15.f, 75.f);
			break;

		case EInputDirection::Left:
			theRange = FVector2D(-105.f, -75.f);
			break;
		case EInputDirection::Right:
			theRange = FVector2D(75.f, 105.f);
			break;

		case EInputDirection::Backward:
			theRange = FVector2D(-165.f, 165.f);
			break;
		case EInputDirection::BackwardLeft:
			theRange = FVector2D(-165.f, -105.f);
			break;
		case EInputDirection::BackwardRight:
			theRange = FVector2D(105.f, 165.f);
			break;
		}
	}
}

#if WITH_EDITOR
void UVMM_InputMappingContext::PostEditChangeProperty(struct FPropertyChangedEvent& InPropertyChangedEvent)
{
	Super::PostEditChangeProperty(InPropertyChangedEvent);

	// Check the property is valid
	if (!InPropertyChangedEvent.Property)
	{
		return;
	}

	// Response the event
	const FName& thePropertyName = InPropertyChangedEvent.Property->GetFName();
	if (thePropertyName == GET_MEMBER_NAME_CHECKED(FInputDefinition, Action))
	{
		InitializeMappingContext();
	}
}

#if 0
void UVMM_InputMappingContext::InitializeInputTag()
{
	// Define initialize input definitions function
	auto OnInitInputTag = [&](TArray<FInputDefinition>& InInputDefinitions)
	{
		for (FInputDefinition& theInputDefinition : InInputDefinitions)
		{
			// Check the input action is valid
			if (!theInputDefinition.IsValid())
			{
				continue;
			}

			// Try find the input tag
			//theInputDefinition.Tag = UGameplayTagsManager::Get().RequestGameplayTag(theInputDefinition.TagName, false);
		}
	};

	// Add input definitions to mapping
	OnInitInputTag(NativeInputDefinitions);
	OnInitInputTag(AbilityInputDefinitions);

	// Flag the data is dirty
	PostEditChange();
	MarkPackageDirty();
}

void UVMM_InputMappingContext::InitializeDataTable()
{
	// Check the data table is valid
	if (GameplayTagDataTable == NULL)
	{
		return;
	}

	// Always clear the table
	GameplayTagDataTable->EmptyTable();

	// Define initialize input table function
	auto OnInitInputTable= [&](const TArray<FInputDefinition>& InInputDefinitions)
	{
		for (const FInputDefinition& theInputDefinition : InInputDefinitions)
		{
			// Check the input action is valid
			if (!theInputDefinition.IsValid())
			{
				continue;
			}

			for (const UInputTrigger* theInputTrigger : theInputDefinition.Action->Triggers)
			{
				if (const UVMM_InputTrigger* theVirtualInputTrigger = static_cast<const UVMM_InputTrigger*>(theInputTrigger))
				{
					// Add the input row to table
// 					FGameplayTagTableRow theGameplayTag(theVirtualInputTrigger->Tag, theInputDefinition.DevComment);
// 					GameplayTagDataTable->AddRow(theInputDefinition.Action->GetFName(), theGameplayTag);
				}
			}
		}
	};

	// Add input definitions to table
	OnInitInputTable(NativeInputDefinitions);
	OnInitInputTable(AbilityInputDefinitions);

	// Flag the data table is dirty
	GameplayTagDataTable->PostEditChange();
	GameplayTagDataTable->MarkPackageDirty();

	// Link the data table to gameplay settings
	if (UGameplayTagsSettings* theGameplayTagsSettings = GetMutableDefault<UGameplayTagsSettings>())
	{
		theGameplayTagsSettings->GameplayTagTableList.AddUnique(GameplayTagDataTable);
		theGameplayTagsSettings->PostEditChange();
		theGameplayTagsSettings->SaveConfig();
	}

	// Try initialize input tags
	InitializeInputTag();
}
#endif

void UVMM_InputMappingContext::InitializeMappingContext()
{
	// Clear the old mappings
	Mappings.Reset();

	// Define initialize input definitions function
	auto OnInitInputDefinitions = [&](const TArray<FInputDefinition>& InInputDefinitions)
	{
		for (const FInputDefinition& theInputDefinition : InInputDefinitions)
		{
			// Check the input action is valid
			if (!theInputDefinition.IsValid())
			{
				continue;
			}

			// Each every keys
			for (const FKeyDefinition& theKeyDefinition : theInputDefinition.Action->KeysDefinition)
			{
				// Add the input definition to mapping
				FEnhancedActionKeyMapping& theMapping = Mappings.AddDefaulted_GetRef();
				theMapping.Action = theInputDefinition.Action;
				theMapping.Key = theKeyDefinition.Key;
				theMapping.Modifiers = theKeyDefinition.Modifiers;
			}
		}
	};

	// Add input definitions to mapping
	OnInitInputDefinitions(LookInputDefinitions);
	OnInitInputDefinitions(MovementInputDefinitions);
	OnInitInputDefinitions(MovementAbilityInputDefinitions);
	OnInitInputDefinitions(NativeInputDefinitions);
	OnInitInputDefinitions(AbilityInputDefinitions);

	// Flag the data table is dirty
	PostEditChange();
	MarkPackageDirty();
}
#endif // WITH_EDITOR
