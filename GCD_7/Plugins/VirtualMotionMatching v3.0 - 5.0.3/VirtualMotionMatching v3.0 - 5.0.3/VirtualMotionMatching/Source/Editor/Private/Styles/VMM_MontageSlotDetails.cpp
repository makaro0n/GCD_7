// Copyright 2021-2022 LeagueTopRyze. All Rights Reserved.

#include "Styles/VMM_MontageSlotDetails.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Animation/Skeleton.h"
#include "EditorStyleSet.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "AnimGraphNode_Base.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Input/STextEntryPopup.h"
#include "PersonaTabs.h"
#include "IDetailChildrenBuilder.h"
#include "DataAssets/VMM_LocomotionData.h"
#include "PropertyCustomizationHelpers.h"
#include "GameFramework/VMM_AnimInstance.h"

#define LOCTEXT_NAMESPACE "VMM_MontageSlotDetails"

/*-----------------------------------------------------------------------------
	FVMM_MontageSlotDataBuilder implementation.
-----------------------------------------------------------------------------*/

FVMM_MontageSlotDataBuilder::FVMM_MontageSlotDataBuilder(USkeleton* InSkeleton, IDetailLayoutBuilder* InDetailLayoutBuilder, const uint32& InSlotDataIndex, const TSharedPtr<IPropertyHandle>& InMontageSlotDataProperty)
	: Skeleton(InSkeleton)
	, DetailLayoutBuilderPtr(InDetailLayoutBuilder)
	, SlotDataIndex(InSlotDataIndex)
	, SlotDataPropertyHandle(InMontageSlotDataProperty)
{}

void FVMM_MontageSlotDataBuilder::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	// Check the details builder is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slot data is valid
	if (!SlotDataPropertyHandle->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theSlostDataArrayProperty = SlotDataPropertyHandle->AsArray();
	const bool bIsArrayProperty = theSlostDataArrayProperty.IsValid();

	// Build node raw
	if (bIsArrayProperty || true)
	{
		// Create the add slot name button
		TSharedRef<SWidget> AddMontageDataButton = PropertyCustomizationHelpers::MakeAddButton(
			FSimpleDelegate::CreateSP(this, &FVMM_MontageSlotDataBuilder::OnAddMontageData), LOCTEXT("AddMontageDataToolTip", "Add Montage Data"), true);

		// Create the insert-delete-duplicate button
		FExecuteAction InsertAction = FExecuteAction::CreateSP(this, &FVMM_MontageSlotDataBuilder::OnInsertMontageData);
		FExecuteAction DeleteAction = FExecuteAction::CreateSP(this, &FVMM_MontageSlotDataBuilder::OnDeleteMontageData);
		FExecuteAction DuplicateAction = FExecuteAction::CreateSP(this, &FVMM_MontageSlotDataBuilder::OnDuplicateMontageData);
		TSharedRef<SWidget> InsertDeleteDuplicateButton = PropertyCustomizationHelpers::MakeInsertDeleteDuplicateButton(InsertAction, DeleteAction, DuplicateAction);

		NodeRow
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("DialogueWaveDetails.HeaderBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
			.Padding(2.0f)
			.HAlign(HAlign_Center)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(this, &FVMM_MontageSlotDataBuilder::GetText)
			.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
			]
			]
			]

		+ SHorizontalBox::Slot()
			.Padding(2.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				AddMontageDataButton
			]

		+ SHorizontalBox::Slot()
			.Padding(2.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				InsertDeleteDuplicateButton
			]

			];
	}
	else
	{
		NodeRow
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("DialogueWaveDetails.HeaderBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
			.Padding(2.0f)
			.HAlign(HAlign_Center)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(this, &FVMM_MontageSlotDataBuilder::GetText)
			.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
			]
			]
			]
			];
	}
}

void FVMM_MontageSlotDataBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	// Check the slot data property is valid
	if (!SlotDataPropertyHandle->IsValidHandle())
	{
		return;
	}

	// Get the slot data children property number
	uint32 theChildrenNumber = 0;
	SlotDataPropertyHandle->GetNumChildren(theChildrenNumber);

#if 0
	// Add all children property to builder
	for (uint32 ChildrenIndex = 0; ChildrenIndex < theChildrenNumber; ChildrenIndex++)
	{
		// Hidden slot name property handle
		TSharedPtr<IPropertyHandle> theSlotNamePropertyHandle = SlotDataPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSkillMontageSlotData, SlotsName));
		if (theSlotNamePropertyHandle == SlotDataPropertyHandle->GetChildHandle(ChildrenIndex))
		{
			continue;
		}

		ChildrenBuilder.AddProperty(SlotDataPropertyHandle->GetChildHandle(ChildrenIndex).ToSharedRef());
	}
#else 
 	TSharedPtr<IPropertyHandle> theDisplayTextPropertyHandle = SlotDataPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSkillMontageSlotData, DisplayText));
 	if (theDisplayTextPropertyHandle->IsValidHandle())
 	{
 		ChildrenBuilder.AddProperty(theDisplayTextPropertyHandle.ToSharedRef());
 	}
#endif

	// Check the slots name property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsNamePropertyHandle = SlotDataPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSkillMontageSlotData, SlotsName));
	if (!theSlotsNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theSlostNameArrayProperty = theSlotsNamePropertyHandle->AsArray();

	// Get the slots name array number
	uint32 theSlotsNameArrayNumber;
	theSlostNameArrayProperty->GetNumElements(theSlotsNameArrayNumber);

	// Create the delegate
	FSimpleDelegate theRebuildDetailsDelegate = FSimpleDelegate::CreateRaw(this, &FVMM_MontageSlotDataBuilder::OnRebuildDetails);
	theSlostNameArrayProperty->SetOnNumElementsChanged(theRebuildDetailsDelegate);

	// Each every slots name
	for (uint32 SlotIndex = 0; SlotIndex < theSlotsNameArrayNumber; SlotIndex++)
	{
		const TSharedPtr<IPropertyHandle> theSlostNameProperty = theSlostNameArrayProperty->GetElement(SlotIndex);

		// Create the slot data builder
		const TSharedRef<FVMM_MontageSlotNodeBuilder> theMontageSlotNodeBuilder
			= MakeShareable(new FVMM_MontageSlotNodeBuilder(Skeleton, DetailLayoutBuilderPtr, SlotDataPropertyHandle, theSlostNameProperty));

		// Add the custom builder
		ChildrenBuilder.AddCustomBuilder(theMontageSlotNodeBuilder);
	}
}

bool FVMM_MontageSlotDataBuilder::InitiallyCollapsed() const
{
	return false;
}

FText FVMM_MontageSlotDataBuilder::GetText() const
{
	// Define the result string
	FString theResultString = SlotDataIndex >= 0 ? ("Index: " + FString::FromInt(SlotDataIndex) + "  /  ") : "";

	// Get the display text
	TSharedPtr<IPropertyHandle> theDisplayTextPropertyHandle = SlotDataPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSkillMontageSlotData, DisplayText));
	if (theDisplayTextPropertyHandle->IsValidHandle())
	{
		FText theDisplayText;
		theDisplayTextPropertyHandle->GetValueAsDisplayText(theDisplayText);

		// Append the text
		theResultString += theDisplayText.ToString();
	}
	else
	{
		// Define the result text
		FString theOutputString = ("Montage Slot Data");

		// Output the result text
		theResultString += theOutputString;
	}

	// Output the result text
	FText theResultText = FText::FromString(MoveTemp(theResultString));
	return theResultText;
}

void FVMM_MontageSlotDataBuilder::OnRebuildDetails()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Force refresh details
	DetailLayoutBuilderPtr->ForceRefreshDetails();
}

bool FVMM_MontageSlotDataBuilder::CanInsertMontageData()
{
	return true;
}

void FVMM_MontageSlotDataBuilder::OnAddMontageData()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slots data property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsDataProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_LocomotionData, MontageSlotsData), UVMM_LocomotionData::StaticClass());
	if (!theSlotsDataProperty->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theMontageDataArrayProperty = theSlotsDataProperty->AsArray();

	// Add the slot name
	theMontageDataArrayProperty->AddItem();
}

void FVMM_MontageSlotDataBuilder::OnInsertMontageData()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slots data property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsDataProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_LocomotionData, MontageSlotsData), UVMM_LocomotionData::StaticClass());
	if (!theSlotsDataProperty->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theMontageDataArrayProperty = theSlotsDataProperty->AsArray();

	// Get the slots data array number
	uint32 theSlotsDataArrayNumber;
	theMontageDataArrayProperty->GetNumElements(theSlotsDataArrayNumber);

	// Get the slot name array index
	const int32& theArrayIndex = SlotDataPropertyHandle->GetIndexInArray();

	// Insert to the array index
	theMontageDataArrayProperty->Insert(theArrayIndex);
}

void FVMM_MontageSlotDataBuilder::OnDeleteMontageData()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slots data property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsDataProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_LocomotionData, MontageSlotsData), UVMM_LocomotionData::StaticClass());
	if (!theSlotsDataProperty->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theMontageDataArrayProperty = theSlotsDataProperty->AsArray();

	// Get the slot name array index
	const int32& theArrayIndex = SlotDataPropertyHandle->GetIndexInArray();

	// Delete the array index item
	theMontageDataArrayProperty->DeleteItem(theArrayIndex);
}

void FVMM_MontageSlotDataBuilder::OnDuplicateMontageData()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slots data property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsDataProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_LocomotionData, MontageSlotsData), UVMM_LocomotionData::StaticClass());
	if (!theSlotsDataProperty->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theMontageDataArrayProperty = theSlotsDataProperty->AsArray();

	// Get the slots data array number
	uint32 theSlotsDataArrayNumber;
	theMontageDataArrayProperty->GetNumElements(theSlotsDataArrayNumber);

	// Get the slot name array index
	const int32& theArrayIndex = SlotDataPropertyHandle->GetIndexInArray();

	// Duplicate to the array index
	theMontageDataArrayProperty->DuplicateItem(theArrayIndex);
}

/*-----------------------------------------------------------------------------
	FVMM_MontageSlotNodeBuilder implementation.
-----------------------------------------------------------------------------*/

FVMM_MontageSlotNodeBuilder::FVMM_MontageSlotNodeBuilder(USkeleton* InSkeleton, IDetailLayoutBuilder* InDetailLayoutBuilder
	, const TSharedPtr<IPropertyHandle>& InMontageSlotDataProperty, const TSharedPtr<IPropertyHandle>& InMontageSlotNameProperty)
	: Skeleton(InSkeleton)
	, DetailLayoutBuilderPtr(InDetailLayoutBuilder)
	, SlotDataPropertyHandle(InMontageSlotDataProperty)
	, SlotNamePropertyHandle(InMontageSlotNameProperty)
{}

void FVMM_MontageSlotNodeBuilder::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	// Check the details builder is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slot data is valid
	if (!SlotDataPropertyHandle->IsValidHandle())
	{
		return;
	}


	// Check the slot name property handle is valid
	if (!SlotNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// this is used for 2 things, to create name, but also for another pop-up box to show off, it's a bit hacky to use this, but I need widget to parent on
	TSharedRef<SWidget> theSlotNodePropertyNameWidget = SlotDataPropertyHandle->CreatePropertyNameWidget();

	// fill combo with groups and slots
	RefreshComboLists();

	// Check the slot name list is valid
	if (SlotNameList.Num() == 0)
	{
		return;
	}

	// Check the slots name is valid
	int32 FoundIndex = SlotNameList.Find(SlotNameComboSelectedName);
	if (!SlotNameComboListItems.IsValidIndex(FoundIndex))
	{
		return;
	}

	TSharedPtr<FString> ComboBoxSelectedItem = SlotNameComboListItems[FoundIndex];

	// Create the add slot name button
	TSharedRef<SWidget> AddSlotNameButton = PropertyCustomizationHelpers::MakeAddButton(
		FSimpleDelegate::CreateSP(this, &FVMM_MontageSlotNodeBuilder::OnAddSlotName), LOCTEXT("AddSlotNameToolTip", "Add Slot Name"), true);

	// Create the insert-delete-duplicate button
	FExecuteAction InsertAction = FExecuteAction::CreateSP(this, &FVMM_MontageSlotNodeBuilder::OnInsertSlotName);
	FExecuteAction DeleteAction = FExecuteAction::CreateSP(this, &FVMM_MontageSlotNodeBuilder::OnDeleteSlotName);
	FExecuteAction DuplicateAction = FExecuteAction::CreateSP(this, &FVMM_MontageSlotNodeBuilder::OnDuplicateSlotName);
	TSharedRef<SWidget> InsertDeleteDuplicateButton = PropertyCustomizationHelpers::MakeInsertDeleteDuplicateButton(InsertAction, DeleteAction, DuplicateAction);

#if 0
	NodeRow
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(SlotNameComboBox, STextComboBox)
			.OptionsSource(&SlotNameComboListItems)
		.OnSelectionChanged(this, &FVMM_MontageSlotNodeBuilder::OnSlotNameChanged)
		.OnComboBoxOpening(this, &FVMM_MontageSlotNodeBuilder::OnSlotListOpening)
		.InitiallySelectedItem(ComboBoxSelectedItem)
		.ContentPadding(2)
		.ToolTipText(FText::FromString(*ComboBoxSelectedItem))
		]

	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("AnimSlotNode_DetailPanelManageButtonLabel", "Anim Slot Manager"))
		.ToolTipText(LOCTEXT("AnimSlotNode_DetailPanelManageButtonToolTipText", "Open Anim Slot Manager to edit Slots and Groups."))
		.OnClicked(this, &FVMM_MontageSlotNodeBuilder::OnOpenAnimSlotManager)
		.Content()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("MeshPaint.FindInCB"))
		]
		]

	+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			AddSlotNameButton
		]

	+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			InsertDeleteDuplicateButton
		]
		];
#else
	// create combo box
	NodeRow
		.NameContent()
		[
			theSlotNodePropertyNameWidget
		]
	.ValueContent()
		.MinDesiredWidth(125.f * 3.f)
		.MaxDesiredWidth(125.f * 3.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(SlotNameComboBox, STextComboBox)
			.OptionsSource(&SlotNameComboListItems)
		.OnSelectionChanged(this, &FVMM_MontageSlotNodeBuilder::OnSlotNameChanged)
		.OnComboBoxOpening(this, &FVMM_MontageSlotNodeBuilder::OnSlotListOpening)
		.InitiallySelectedItem(ComboBoxSelectedItem)
		.ContentPadding(2)
		.ToolTipText(FText::FromString(*ComboBoxSelectedItem))
		]

	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("AnimSlotNode_DetailPanelManageButtonLabel", "Anim Slot Manager"))
		.ToolTipText(LOCTEXT("AnimSlotNode_DetailPanelManageButtonToolTipText", "Open Anim Slot Manager to edit Slots and Groups."))
		.OnClicked(this, &FVMM_MontageSlotNodeBuilder::OnOpenAnimSlotManager)
		.Content()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("MeshPaint.FindInCB"))
		]
		]

	+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			AddSlotNameButton
		]

	+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			InsertDeleteDuplicateButton
		]
		];
#endif
}

void FVMM_MontageSlotNodeBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
}

FReply FVMM_MontageSlotNodeBuilder::OnOpenAnimSlotManager()
{
	OnInvokeTab.ExecuteIfBound(FPersonaTabs::SkeletonSlotNamesID);
	return FReply::Handled();
}

void FVMM_MontageSlotNodeBuilder::OnSlotListOpening()
{
	// Refresh Slot Names, in case we used the Anim Slot Manager to make changes.
	RefreshComboLists(true);
}

void FVMM_MontageSlotNodeBuilder::OnSlotNameChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	// if it's set from code, we did that on purpose
	if (SelectInfo != ESelectInfo::Direct)
	{
		int32 ItemIndex = SlotNameComboListItems.Find(NewSelection);
		if (ItemIndex != INDEX_NONE)
		{
			SlotNameComboSelectedName = SlotNameList[ItemIndex];
			if (SlotNameComboBox.IsValid())
			{
				SlotNameComboBox->SetToolTipText(FText::FromString(*NewSelection));
			}

			if (Skeleton->ContainsSlotName(SlotNameComboSelectedName))
			{
				// trigger transaction 
				//const FScopedTransaction Transaction(LOCTEXT("ChangeSlotNodeName", "Change Collision Profile"));
				// set profile set up
				ensure(SlotNamePropertyHandle->SetValue(SlotNameComboSelectedName.ToString()) == FPropertyAccess::Result::Success);
			}
		}
	}
}

void FVMM_MontageSlotNodeBuilder::RefreshComboLists(bool bOnlyRefreshIfDifferent /*= false*/)
{
	SlotNamePropertyHandle->GetValue(SlotNameComboSelectedName);

	// Check the skeleton is valid
	if (Skeleton == nullptr)
	{
		return;
	}

	// Make sure slot node exists in our skeleton.
	Skeleton->RegisterSlotNode(SlotNameComboSelectedName);

	// Refresh Slot Names
	{
		TArray< TSharedPtr< FString > > NewSlotNameComboListItems;
		TArray< FName > NewSlotNameList;
		bool bIsSlotNameListDifferent = false;

		const TArray<FAnimSlotGroup>& SlotGroups = Skeleton->GetSlotGroups();
		for (auto SlotGroup : SlotGroups)
		{
			int32 Index = 0;
			for (auto SlotName : SlotGroup.SlotNames)
			{
				NewSlotNameList.Add(SlotName);

				FString ComboItemString = FString::Printf(TEXT("%s.%s"), *SlotGroup.GroupName.ToString(), *SlotName.ToString());
				NewSlotNameComboListItems.Add(MakeShareable(new FString(ComboItemString)));

				bIsSlotNameListDifferent = bIsSlotNameListDifferent || (!SlotNameComboListItems.IsValidIndex(Index) || (SlotNameComboListItems[Index] != NewSlotNameComboListItems[Index]));
				Index++;
			}
		}

		// Refresh if needed
		if (bIsSlotNameListDifferent || !bOnlyRefreshIfDifferent || (NewSlotNameComboListItems.Num() == 0))
		{
			SlotNameComboListItems = NewSlotNameComboListItems;
			SlotNameList = NewSlotNameList;

			if (SlotNameComboBox.IsValid())
			{
				if (Skeleton->ContainsSlotName(SlotNameComboSelectedName))
				{
					int32 FoundIndex = SlotNameList.Find(SlotNameComboSelectedName);
					TSharedPtr<FString> ComboItem = SlotNameComboListItems[FoundIndex];

					SlotNameComboBox->SetSelectedItem(ComboItem);
					SlotNameComboBox->SetToolTipText(FText::FromString(*ComboItem));
				}
				SlotNameComboBox->RefreshOptions();
			}
		}
	}
}

bool FVMM_MontageSlotNodeBuilder::CanInsertSlotName()
{
	return true;
}

void FVMM_MontageSlotNodeBuilder::OnAddSlotName()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slots name property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsNamePropertyHandle = SlotDataPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSkillMontageSlotData, SlotsName));
	if (!theSlotsNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theSlostNameArrayProperty = theSlotsNamePropertyHandle->AsArray();

	// Add the slot name
	theSlostNameArrayProperty->AddItem();

	//RefreshComboLists();
	//DetailLayoutBuilderPtr->ForceRefreshDetails();
}

void FVMM_MontageSlotNodeBuilder::OnInsertSlotName()
{
	// Check the slot name property handle is valid
	if (!SlotNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slots name property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsNamePropertyHandle = SlotDataPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSkillMontageSlotData, SlotsName));
	if (!theSlotsNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theSlostNameArrayProperty = theSlotsNamePropertyHandle->AsArray();

	// Get the slots data array number
	uint32 theSlotsDataArrayNumber;
	theSlostNameArrayProperty->GetNumElements(theSlotsDataArrayNumber);

	// Get the slot name array index
	const int32& theArrayIndex = SlotNamePropertyHandle->GetIndexInArray();

	// Insert to the array index
	theSlostNameArrayProperty->Insert(theArrayIndex);
}

void FVMM_MontageSlotNodeBuilder::OnDeleteSlotName()
{
	// Check the slot name property handle is valid
	if (!SlotNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slots name property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsNamePropertyHandle = SlotDataPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSkillMontageSlotData, SlotsName));
	if (!theSlotsNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theSlostNameArrayProperty = theSlotsNamePropertyHandle->AsArray();

	// Get the slots data array number
	uint32 theSlotsDataArrayNumber;
	theSlostNameArrayProperty->GetNumElements(theSlotsDataArrayNumber);

	// Get the slot name array index
	const int32& theArrayIndex = SlotNamePropertyHandle->GetIndexInArray();

	// Delete the array index item
	theSlostNameArrayProperty->DeleteItem(theArrayIndex);
}

void FVMM_MontageSlotNodeBuilder::OnDuplicateSlotName()
{
	// Check the slot name property handle is valid
	if (!SlotNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Check the slots name property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsNamePropertyHandle = SlotDataPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSkillMontageSlotData, SlotsName));
	if (!theSlotsNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theSlostNameArrayProperty = theSlotsNamePropertyHandle->AsArray();

	// Get the slots data array number
	uint32 theSlotsDataArrayNumber;
	theSlostNameArrayProperty->GetNumElements(theSlotsDataArrayNumber);

	// Get the slot name array index
	const int32& theArrayIndex = SlotNamePropertyHandle->GetIndexInArray();

	// Duplicate to the array index
	theSlostNameArrayProperty->DuplicateItem(theArrayIndex);
}

/*-----------------------------------------------------------------------------
	FVMM_MontageSlotDetails implementation.
-----------------------------------------------------------------------------*/

FVMM_MontageSlotDetails::FVMM_MontageSlotDetails(FOnInvokeTab InOnInvokeTab)
	: OnInvokeTab(InOnInvokeTab)
{}

TSharedRef<IDetailCustomization> FVMM_MontageSlotDetails::MakeInstance(FOnInvokeTab InOnInvokeTab)
{
	return MakeShareable(new FVMM_MontageSlotDetails(InOnInvokeTab));
}

void FVMM_MontageSlotDetails::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	// Cache the details reference
	DetailLayoutBuilderPtr = &DetailBuilder;

	// Edit the slot name group category
	IDetailCategoryBuilder& theSlotsDataCategoryBuilder = DetailBuilder.EditCategory(TEXT("Montage Attributes"), FText::GetEmpty(), ECategoryPriority::Default);

	// Check the skeleton property handle is valid
	const TSharedPtr<IPropertyHandle> theSkeletonProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_LocomotionData, Skeleton), UVMM_LocomotionData::StaticClass());
	if (!theSkeletonProperty->IsValidHandle())
	{
		return;
	}

	// Convert the object to skeleton
	UObject* theSkeletonPropertyObject;
	theSkeletonProperty->GetValue(theSkeletonPropertyObject);
	USkeleton* theSkeleton = Cast<USkeleton>(theSkeletonPropertyObject);

	// Check the slots data property handle is valid
	const TSharedPtr<IPropertyHandle> theSlotsDataProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_LocomotionData, MontageSlotsData), UVMM_LocomotionData::StaticClass());
	if (!theSlotsDataProperty->IsValidHandle())
	{
		return;
	}

	// Mark hidden the property handle
	theSlotsDataProperty->MarkHiddenByCustomization();

	// Get the slots data array handle
	const TSharedPtr<IPropertyHandleArray> theSlostDataArrayProperty = theSlotsDataProperty->AsArray();
	if (!theSlostDataArrayProperty.IsValid())
	{
		return;
	}

	// Get the slots data array number
	uint32 theSlotsDataArrayNumber;
	theSlostDataArrayProperty->GetNumElements(theSlotsDataArrayNumber);

	// Create the delegate
	FSimpleDelegate theRebuildDetailsDelegate = FSimpleDelegate::CreateRaw(this, &FVMM_MontageSlotDetails::OnRebuildDetails);
	theSlostDataArrayProperty->SetOnNumElementsChanged(theRebuildDetailsDelegate);

	// Each every array index
	for (uint32 ArrayIndex = 0; ArrayIndex < theSlotsDataArrayNumber; ArrayIndex++)
	{
		// Get the slot data handle in slots data
		const TSharedPtr<IPropertyHandle> theSlotDataProperty = theSlostDataArrayProperty->GetElement(ArrayIndex);

		// Create the slot data builder
		const TSharedRef<FVMM_MontageSlotDataBuilder> theMontageSlotDataBuilder
			= MakeShareable(new FVMM_MontageSlotDataBuilder(theSkeleton, DetailLayoutBuilderPtr, int32(ArrayIndex), theSlotDataProperty));

		// Add the custom builder
		theSlotsDataCategoryBuilder.AddCustomBuilder(theMontageSlotDataBuilder);
	}
}

void FVMM_MontageSlotDetails::OnRebuildDetails()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Force refresh details
	DetailLayoutBuilderPtr->ForceRefreshDetails();
}


/*-----------------------------------------------------------------------------
	FVMM_AnimInstanceMontageSlotDetails Implementation.
-----------------------------------------------------------------------------*/

FVMM_AnimInstanceMontageSlotDetails::FVMM_AnimInstanceMontageSlotDetails(FOnInvokeTab InOnInvokeTab)
	: OnInvokeTab(InOnInvokeTab)
{}

TSharedRef<IDetailCustomization> FVMM_AnimInstanceMontageSlotDetails::MakeInstance(FOnInvokeTab InOnInvokeTab)
{
	return MakeShareable(new FVMM_AnimInstanceMontageSlotDetails(InOnInvokeTab));
}

void FVMM_AnimInstanceMontageSlotDetails::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	// Cache the details reference
	DetailLayoutBuilderPtr = &DetailBuilder;

	// Edit the slot name group category
	IDetailCategoryBuilder& theSlotsDataCategoryBuilder = DetailBuilder.EditCategory(TEXT("Animation Layer"), FText::GetEmpty(), ECategoryPriority::Default);

	// Check the skeleton property handle is valid
	const TSharedPtr<IPropertyHandle> theSkeletonProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_AnimInstance, EditorSkeleton), UVMM_AnimInstance::StaticClass());
	if (!theSkeletonProperty->IsValidHandle())
	{
		return;
	}

	// Convert the object to skeleton
	UObject* theSkeletonPropertyObject;
	theSkeletonProperty->GetValue(theSkeletonPropertyObject);
	USkeleton* theSkeleton = Cast<USkeleton>(theSkeletonPropertyObject);

	// Lower body layer data
	{
		// Check the slots data property handle is valid
		const TSharedPtr<IPropertyHandle> theSlotDataProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_AnimInstance, LowerBodySlotData), UVMM_AnimInstance::StaticClass());
		if (!theSlotDataProperty->IsValidHandle())
		{
			return;
		}

		// Mark hidden the property handle
		theSlotDataProperty->MarkHiddenByCustomization();

		// Create the slot data builder
		const TSharedRef<FVMM_MontageSlotDataBuilder> theMontageSlotDataBuilder
			= MakeShareable(new FVMM_MontageSlotDataBuilder(theSkeleton, DetailLayoutBuilderPtr, INDEX_NONE, theSlotDataProperty));

		// Add the custom builder
		theSlotsDataCategoryBuilder.AddCustomBuilder(theMontageSlotDataBuilder);
	}

	// Upper body layer data
	{
		// Check the slots data property handle is valid
		const TSharedPtr<IPropertyHandle> theSlotDataProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_AnimInstance, UpperBodySlotData), UVMM_AnimInstance::StaticClass());
		if (!theSlotDataProperty->IsValidHandle())
		{
			return;
		}

		// Mark hidden the property handle
		theSlotDataProperty->MarkHiddenByCustomization();

		// Create the slot data builder
		const TSharedRef<FVMM_MontageSlotDataBuilder> theMontageSlotDataBuilder
			= MakeShareable(new FVMM_MontageSlotDataBuilder(theSkeleton, DetailLayoutBuilderPtr, INDEX_NONE, theSlotDataProperty));

		// Add the custom builder
		theSlotsDataCategoryBuilder.AddCustomBuilder(theMontageSlotDataBuilder);
	}

	// Whole body layer data
	{
		// Check the slots data property handle is valid
		const TSharedPtr<IPropertyHandle> theSlotDataProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVMM_AnimInstance, WholeBodySlotData), UVMM_AnimInstance::StaticClass());
		if (!theSlotDataProperty->IsValidHandle())
		{
			return;
		}

		// Mark hidden the property handle
		theSlotDataProperty->MarkHiddenByCustomization();

		// Create the slot data builder
		const TSharedRef<FVMM_MontageSlotDataBuilder> theMontageSlotDataBuilder
			= MakeShareable(new FVMM_MontageSlotDataBuilder(theSkeleton, DetailLayoutBuilderPtr, INDEX_NONE, theSlotDataProperty));

		// Add the custom builder
		theSlotsDataCategoryBuilder.AddCustomBuilder(theMontageSlotDataBuilder);
	}
}

void FVMM_AnimInstanceMontageSlotDetails::OnRebuildDetails()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Force refresh details
	DetailLayoutBuilderPtr->ForceRefreshDetails();
}

#undef LOCTEXT_NAMESPACE
