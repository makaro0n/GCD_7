// Copyright Epic Games, Inc. All Rights Reserved.

#include "Styles/VMM_SettingsCustomization.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "VMM_Settings.h"
#include "DataRegistrySettings.h"
#include "GameplayTagsSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/InputSettings.h"
#include "Input/VMM_InputComponent.h"

#define LOCTEXT_NAMESPACE "VMM_SettingsCustomization"

//////////////////////////////////////////////////////////////////////////
// FVMM_SettingsCustomization
namespace FVMM_SettingsCustomizationConstants
{
	const FText DisabledTip = LOCTEXT("VirtualMotionMatchingToolTip", "This VMM source.");
}

TSharedRef<IDetailCustomization> FVMM_SettingsCustomization::MakeInstance()
{
	return MakeShareable(new FVMM_SettingsCustomization());
}

FVMM_SettingsCustomization::FVMM_SettingsCustomization()
{}

void FVMM_SettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	IDetailCategoryBuilder& thePerformanceCategory = DetailLayout.EditCategory(TEXT("Editor"));

	thePerformanceCategory.AddCustomRow(LOCTEXT("VMM_Editor", "Virtual Animation Skill Editor Function"))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("InitializeProjectSettings", "Initialize Project settings"))
		.ToolTipText(LOCTEXT("InitializeProjectSettings_Tooltip", "Initialize data registry path, Gameplay tags data table, default input components."))
		.OnClicked(this, &FVMM_SettingsCustomization::InitializeProjectSettings)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("CollectSkillDataAsset", "Collect Skill Data Assets"))
		.ToolTipText(LOCTEXT("CollectSkillDataAssetButton_Tooltip", "Collect all animation skill data assets, sort and cache them in virtual data."))
		.OnClicked(this, &FVMM_SettingsCustomization::CollectSkillDataAssets)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("SaveConfigData", "Save Config"))
		.ToolTipText(LOCTEXT("SaveConfigButton_Tooltip", "Save the config data."))
		.OnClicked(this, &FVMM_SettingsCustomization::SaveConfig)
		]
	];
}

FReply FVMM_SettingsCustomization::InitializeProjectSettings()
{
	if (UDataRegistrySettings* theSettings = GetMutableDefault<UDataRegistrySettings>())
	{
		bool bHasPath = false;
		FDirectoryPath theNewPath;
		theNewPath.Path = FString(TEXT("/VirtualMotionMatching/Mannequin/Animations/DataAssets"));

		for (const FDirectoryPath& thePath : theSettings->DirectoriesToScan)
		{
			if (thePath.Path == theNewPath.Path)
			{
				bHasPath = true;
				break;
			}
		}

		if (!bHasPath)
		{
			theSettings->DirectoriesToScan.Add(theNewPath);
			theSettings->SaveConfig();
#if ENGINE_MAJOR_VERSION > 4
			theSettings->TryUpdateDefaultConfigFile(theSettings->GetDefaultConfigFilename());
#else
			theSettings->UpdateDefaultConfigFile(theSettings->GetDefaultConfigFilename());
#endif // ENGINE_MAJOR_VERSION > 4
		}
	}
	if (UGameplayTagsSettings* theSettings = GetMutableDefault<UGameplayTagsSettings>())
	{
		FAssetRegistryModule& theAssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FARFilter theFilter;
		theFilter.PackagePaths.Add("/VirtualMotionMatching/Blueprints/Abilities");
		theFilter.ClassNames.Add(UDataTable::StaticClass()->GetFName());
		theFilter.bRecursivePaths = true;
		theFilter.bRecursiveClasses = true;

		TArray<FAssetData> theAssetDatas;
		theAssetRegistryModule.Get().GetAssets(theFilter, theAssetDatas);
		for (FAssetData& theAssetData : theAssetDatas)
		{
			theSettings->GameplayTagTableList.AddUnique(theAssetData.ToSoftObjectPath());
		}
		theSettings->SaveConfig();
#if ENGINE_MAJOR_VERSION > 4
		theSettings->TryUpdateDefaultConfigFile(theSettings->GetDefaultConfigFilename());
#else
		theSettings->UpdateDefaultConfigFile(theSettings->GetDefaultConfigFilename());
#endif // ENGINE_MAJOR_VERSION > 4
	}
// 	if (UInputSettings* theSettings = GetMutableDefault<UInputSettings>())
// 	{
// 		theSettings->DefaultPlayerInputClass = UEnhancedPlayerInput::StaticClass();
// 		theSettings->DefaultInputComponentClass = UVMM_InputComponent::StaticClass();
// 		theSettings->SaveConfig();
// #if ENGINE_MAJOR_VERSION > 4
// 		theSettings->TryUpdateDefaultConfigFile(theSettings->GetDefaultConfigFilename());
// #else
// 		theSettings->UpdateDefaultConfigFile(theSettings->GetDefaultConfigFilename());
// #endif // ENGINE_MAJOR_VERSION > 4
// 	}
	// Success
	return FReply::Handled();
}

FReply FVMM_SettingsCustomization::CollectSkillDataAssets()
{
	// Response setting event
	if (UVMM_Settings* theMutableDefaultSettings = GetMutableDefault<UVMM_Settings>())
	{
		theMutableDefaultSettings->CollectSkillDataAssets();
	}

	// Success
	return FReply::Handled();
}

FReply FVMM_SettingsCustomization::SaveConfig()
{
	// Response setting event
	if (UVMM_Settings* theMutableDefaultSettings = GetMutableDefault<UVMM_Settings>())
	{
		theMutableDefaultSettings->ResponseSaveConfig();
	}

	// Success
	return FReply::Handled();
}
//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE