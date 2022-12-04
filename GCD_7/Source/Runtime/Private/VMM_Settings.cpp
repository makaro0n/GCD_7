// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "VMM_Settings.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_EDITOR
#include "Misc/ScopedSlowTask.h"
#endif // WITH_EDITOR
#include "Input/VMM_InputMappingContext.h"
#include "DataAssets/VMM_LocomotionData.h"


#define LOCTEXT_NAMESPACE "VMM_Settings"


UVMM_Settings::UVMM_Settings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TrajectoryCount(5)
	, TrajectoryTime(0.2f)
	, TrajectoryArrowSize(20.5)
	, TrajectoryArrowThickness(1.5f)
	, TrajectoryArrowColor(FLinearColor::Black)
	, TrajectorySphereRadius(2.5f)
	, TrajectorySphereSegments(12)
	, TrajectorySphereThickness(1.5f)
	, TrajectorySpehreColor(FLinearColor::Black)
{
	ConfigFileName = GetDefaultConfigFilename();
#if WITH_EDITORONLY_DATA
	bRegisterDataName = true;
	bRegisterMontageSlotStyle = true;
#endif 

	// Check the motion curve enum is valid
	UEnum* theEnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EMotionCurveType"), true);
	if (theEnumPtr == nullptr)
	{
		return;
	}

	// Each every motion matching curves type
	for (int32 i = 0; i < int32(EMotionCurveType::MAX); i++)
	{
		AnimCurvesNameMap.FindOrAdd(EMotionCurveType(i), *theEnumPtr->GetNameStringByIndex(i));
	}
}

#pragma region Config
void UVMM_Settings::ResponseSaveConfig()
{
	SaveConfig();

#if ENGINE_MAJOR_VERSION > 4
	TryUpdateDefaultConfigFile(ConfigFileName);
#else
	UpdateDefaultConfigFile(ConfigFileName);
#endif // ENGINE_MAJOR_VERSION > 4
}
#pragma endregion


#pragma region Data
const UVMM_LocomotionData* UVMM_Settings::GetDataAsset(const int32 InDataIndex) const
{
	// Check the data index is valid
	if (!AnimSkillDataAssets.IsValidIndex(InDataIndex))
	{
		return nullptr;
	}

#if 1 // Async?
	// Cache the start load time
	const double theStartLoadTime = FPlatformTime::Seconds();

	// Request sync load, should use Async load from active skill
	FStreamableManager& theStreamableManager = UAssetManager::GetStreamableManager();
	UObject* theDataAsset = theStreamableManager.LoadSynchronous(AnimSkillDataAssets[InDataIndex]);

	// Calculate the load time
	const float theLoadTime = FPlatformTime::Seconds() - theStartLoadTime;
#endif

	// Return the result
	return Cast<UVMM_LocomotionData>(theDataAsset);
}


const FLocomotionParams* UVMM_Settings::GetParams(const FVirtualSkillParamsGroup& InSkillParamsGroup) const
{
	// Check the skill data asset is valid
	const UVMM_LocomotionData* theSkillData = GetDataAsset(InSkillParamsGroup.DataIndex);
	if (theSkillData == nullptr)
	{
		return nullptr;
	}

	// Return the result
	return static_cast<const FLocomotionParams*>(theSkillData->GetParamsByIndex(InSkillParamsGroup.ParamsIndex));
}

const FSoftObjectPath* UVMM_Settings::GetCharacterAttributeTable(const uint16& InDataIndex) const
{
	// Check the data index is valid
	if (!CharacterAttributeTables.IsValidIndex(InDataIndex))
	{
		return nullptr;
	}

	// Return the result
	return &CharacterAttributeTables[InDataIndex];
}

#if WITH_EDITOR
void UVMM_Settings::CollectSkillDataAssets()
{
	FScopedSlowTask theCollectTask(100.f, LOCTEXT("VMM_CollectTask", "Collect all animation skill data assets, sort and cache them in virtual data."));
	theCollectTask.MakeDialog(true);

	// Define the collect number variable
	uint16 theCollectNumber = 0;

	// Flag the task progress
	theCollectTask.EnterProgressFrame(75.f);

	// Clear cached data
	AnimSkillDataAssets.Reset();
#if 1
	// Each all animation skill data asset reference
	for (TObjectIterator<UVMM_LocomotionData> SkillDataIterator; SkillDataIterator; ++SkillDataIterator)
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (theCollectTask.ShouldCancel())
		{
			break;
		}

		// Check the data asset is valid
		UVMM_LocomotionData* theSkillDataAsset = *SkillDataIterator;
		if (theSkillDataAsset)
		{
			if (bRegisterDataName)
			{
				theSkillDataAsset->SetDataName(theSkillDataAsset->GetFName());
			}
			theSkillDataAsset->SortParamsIndex();
			theSkillDataAsset->SetDataIndex(theCollectNumber);
			theSkillDataAsset->PostEditChange();
			theSkillDataAsset->MarkPackageDirty();
#if 1 // Use soft object path
			AnimSkillDataAssets.Add(FSoftObjectPath(theSkillDataAsset));
#else
			AnimSkillDataAssets.Add(theSkillDataAsset);
#endif // Use soft object path
			++theCollectNumber;
		}
	}
#endif
	// Flag the task progress
	theCollectTask.EnterProgressFrame(25.f);

	// Save the config data
	ResponseSaveConfig();
}
#endif // WITH_EDITOR
#pragma endregion


#pragma region Trajectory
void UVMM_Settings::TrajectoryAgentTransform(UObject* InInstance, const FTransform& InAgentTransform, const FTransform* InLastAgentTransform)
{
	// Get the transform data
	const FVector theAgentLocation = InAgentTransform.GetLocation();
	const FQuat theAgentRotation = InAgentTransform.GetRotation();

	// Draw debug
	UKismetSystemLibrary::DrawDebugSphere(InInstance, theAgentLocation, TrajectorySphereRadius
		, TrajectorySphereSegments, TrajectorySpehreColor, -1.f, TrajectorySphereThickness);
	UKismetSystemLibrary::DrawDebugArrow(InInstance, theAgentLocation, theAgentLocation + theAgentRotation.Vector() * TrajectoryArrowSize
		, TrajectoryArrowSize, TrajectoryArrowColor, -1.f, TrajectoryArrowThickness);

#if 1
	// Draw both lines
	if (InLastAgentTransform != nullptr)
	{
		// Get the last transform data
		const FVector theLastAgentLocation = InLastAgentTransform->GetLocation();
		const FQuat theLastAgentRotation = InLastAgentTransform->GetRotation();

		// Calculate both distance
		const float theDistance = (theAgentLocation - theLastAgentLocation).Size();
		const FVector theDirection = (theAgentLocation - theLastAgentLocation).GetSafeNormal();

		// Each every point
		const int32 thePointNumber = 3;
		for (int32 PointIndex = 1; PointIndex < thePointNumber; PointIndex++)
		{
			const FVector theDrawLoc = theLastAgentLocation + theDirection * (theDistance / thePointNumber * PointIndex);
			UKismetSystemLibrary::DrawDebugSphere(InInstance, theDrawLoc, 0.5f
				, TrajectorySphereSegments, TrajectorySpehreColor, -1.f, TrajectorySphereThickness);
		}
	}
#endif
}
#pragma endregion

#undef LOCTEXT_NAMESPACE