// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;

//////////////////////////////////////////////////////////////////////////
// FVMM_SettingsCustomization


class FVMM_SettingsCustomization : public IDetailCustomization
{
public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	VIRTUALMOTIONMATCHINGEDITOR_API static TSharedRef<IDetailCustomization> MakeInstance();

	FVMM_SettingsCustomization();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface

private:

	/** Initialize project settings. */
	FReply InitializeProjectSettings();

	/** Collect all animation skill data assets, sort and cache them in virtual data. */
	FReply CollectSkillDataAssets();

	/** Save config data. */
	FReply SaveConfig();
};