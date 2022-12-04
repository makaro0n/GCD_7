
#include "VirtualMotionMatchingEditor.h"
#include "VMM_Settings.h"
#include "Textures/SlateIcon.h"
#include "AnimGraphCommands.h"
#include "Modules/ModuleManager.h"
#include "EditorModeRegistry.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ISettingsModule.h"
#include "DataAssets/VMM_LocomotionData.h"
#include "GameFramework/VMM_AnimInstance.h"
#include "Styles/VMM_MontageSlotDetails.h"

DEFINE_LOG_CATEGORY(VirtualMotionMatchingEditor);

#define LOCTEXT_NAMESPACE "FVirtualMotionMatchingEditor"

void FVirtualMotionMatchingEditor::StartupModule()
{
	UE_LOG(VirtualMotionMatchingEditor, Warning, TEXT("VirtualMotionMatchingEditor has started!"));

	// Get the property editor module
	FPropertyEditorModule& thePropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");	

	// Register settings details view panel
	thePropertyModule.RegisterCustomClassLayout(
		UVMM_Settings::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FVMM_SettingsCustomization::MakeInstance));

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Virtual Motion Matching",
			LOCTEXT("VirtualMotionMatchingSettingsName", "VirtualMotionMatching"),
			LOCTEXT("VirtualMotionMatchingSettingsDescription", "Configure the Virtual Animation Skills plug-in."),
			GetMutableDefault<UVMM_Settings>()
		);
	}

	// Register montage slot name
    // Get the replicated data asset from global manager
	if (UVMM_Settings* theMutableDefaultSettings = GetMutableDefault<UVMM_Settings>())
	{
		if (theMutableDefaultSettings->bRegisterMontageSlotStyle)
		{
			thePropertyModule.RegisterCustomClassLayout(UVMM_LocomotionData::StaticClass()->GetFName()
				, FOnGetDetailCustomizationInstance::CreateStatic(&FVMM_MontageSlotDetails::MakeInstance, FOnInvokeTab()));
			thePropertyModule.RegisterCustomClassLayout(UVMM_AnimInstance::StaticClass()->GetFName()
				, FOnGetDetailCustomizationInstance::CreateStatic(&FVMM_AnimInstanceMontageSlotDetails::MakeInstance, FOnInvokeTab()));
		}
	}
	// Notify the module changed
	thePropertyModule.NotifyCustomizationModuleChanged();
}

void FVirtualMotionMatchingEditor::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Virtual Motion Matching");
	}
	UE_LOG(VirtualMotionMatchingEditor, Warning, TEXT("VirtualMotionMatchingEditor has shut down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVirtualMotionMatchingEditor, VirtualMotionMatchingEditor)
