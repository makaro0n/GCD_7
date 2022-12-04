// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "DataAssets/VMM_LocomotionData.h"
#include "Kismet/KismetStringLibrary.h"
#include "VMM_Settings.h"

#if WITH_EDITOR
#include "Misc/ScopedSlowTask.h"
#include "EditorUtilityLibrary.h"
#include "AnimationBlueprintLibrary.h"
#endif // WITH_EDITOR

static const FName CURVENAME_FootWeight = FName(TEXT("FootWeight"));
static const FName CURVENAME_FootPosition = FName(TEXT("FootPosition"));

#define LOCTEXT_NAMESPACE "VMM_LocomotionData"
UVMM_LocomotionData::UVMM_LocomotionData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)

	, DataName(NAME_None)
	, DataIndex(0)

	, FootWeightCurveName(CURVENAME_FootWeight)
	, FootPositionCurveName(CURVENAME_FootPosition)
	, AimOffset(nullptr)

	, ResponseRandomTransitionIndexRange(0)

	, bResponseTurnInPlace(true)
	, ResponseTurnInPlaceIntervalTime(0.2f)
	, ResponseTurnInPlaceAngleRange(FVector2D(-70.f, 70.f))
	, ResponseRandomTurnInPlaceIndexRange(INDEX_NONE)

	, bResponseStartStep(true)
	, bResponseStartStepWaitTurn(false)
	, bResponseStartStepWaitFootLock(false)
	, ResponseStartAfterTurnTime(0.11f)
	, ResponseStartAfterStopTime(0.2f)

	, bResponseCircle(true)
	, bResponseCircleInLookingMode(true)
	, ResponseCicleAngle(5.f)

	, bResponsePivotStep(true)
	, bResponsePivotStop(true)
	, ResponsePivotAngle(120.f)
	, ResponsePivotIntervalTime(0.1f)
	, ResponsePivotAsMovingWeightFromStop(0.5f)
	, ResponsePivotAsMovingBlendTimeFromStop(0.2f)

	, bResponseStopStep(true)
	, GlobalJumpHeight(0.f)
	, BasePose(nullptr)
{
	// Initialize pose distance curves name
	{
		FName& theLeftFootName = PoseDistanceCurvesNameMap.FindOrAdd(-1.f);
		theLeftFootName = "Foot_L";
	}
	{
		FName& theLeftFootName = PoseDistanceCurvesNameMap.FindOrAdd(1.f);
		theLeftFootName = "Foot_R";
	}

#if WITH_EDITORONLY_DATA
	Skeleton = nullptr;
	PhysicsAsset = nullptr;
	MontageSlotsData.AddDefaulted();
	for (int32 Index = 0; Index < int32(ELocomotionStatus::MAX); Index++)
	{
		int32& theValue = MontageSlotDataIndexMap.FindOrAdd(ELocomotionStatus(Index));
		theValue = Index;
	}
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
void UVMM_LocomotionData::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(InPropertyChangedEvent);

	// Check the property is valid
	if (!InPropertyChangedEvent.Property)
	{
		return;
	}

	// Get the property name
	const FName thePropertyName = InPropertyChangedEvent.Property->GetFName();

	// Handle event
	if (thePropertyName == GET_MEMBER_NAME_CHECKED(UVMM_LocomotionData, GaitAttributesData))
	{
		if (GaitAttributesData.Num() > 0)
		{
			GaitAttributesData.Last().RandomIndexRange = FIntPoint(GaitAttributesData.Num() - 1);
		}
	}
}
#endif // WITH_EDITOR

#pragma region Transition
const FLocomotionParams* UVMM_LocomotionData::GetTransitionParams(const int32& InFootIndex, const FName* InName) const
{
	// Get the transition animation params
	const FLocomotionTransitionAnims* theAnimParams = GetTransitionAnimParams(InFootIndex, InName);

	// Return the result
	return GetTransitionParams(theAnimParams, INDEX_NONE);
}

const FLocomotionParams* UVMM_LocomotionData::GetTransitionParams(const FLocomotionTransitionAnims* theAnimParams, const int32 InTypeIndex) const
{
	check(theAnimParams);

	// Check the params is valid
	if (theAnimParams->MultiParams.Num() > 0)
	{
		// Random the params index
		const int32 theRandomIndex = InTypeIndex >= 0 ? InTypeIndex : FMath::RandRange(0, theAnimParams->MultiParams.Num() - 1);

		// Check the random params is valid
		if (theAnimParams->MultiParams.IsValidIndex(theRandomIndex) && theAnimParams->MultiParams[theRandomIndex].IsValid())
		{
			return &theAnimParams->MultiParams[theRandomIndex];
		}
		else if (theAnimParams->MultiParams[0].IsValid())
		{
			return &theAnimParams->MultiParams[0];
		}
	}

	// Failed
	return nullptr;
}

const FLocomotionTransitionAnims* UVMM_LocomotionData::GetTransitionAnimParams(const int32& InFootIndex, const FName* InName) const
{
	// Check the desired name is valid
	if (InName != nullptr)
	{
		const FLocomotionTransitionAnims* theParams = TransitionParamsMap.Find(*InName);
		if (theParams != nullptr)
		{
			return theParams;
		}
	}

	// Calculate the params index
	int32 theParamsIndex = InFootIndex;

	// Check the default name is valid
	const FName theDefaultName = *FString::FromInt(theParamsIndex);
	const FLocomotionTransitionAnims* theParams = TransitionParamsMap.Find(theDefaultName);
	if (theParams != nullptr)
	{
		return theParams;
	}

	// Return the result
	return nullptr;
}
#pragma endregion


#pragma region TurnInPlace
TAutoConsoleVariable<int32> CVarLocomotionData_LockTurnInPlace(TEXT("a.AnimSkill.Locomotion.LockTurnInPlace"), INDEX_NONE
	, TEXT("Lock the desired Params direction"));

const FLocomotionParams* UVMM_LocomotionData::GetTurnInPlaceParams(const FIntPoint& InGaitAxisRange, const EAngleDirection& InAngleDirection) const
{
	// Convert the angle direction to int
	int32 theAngleDirectionIndex = int32(InAngleDirection);

#if WITH_EDITOR // Response lock event
	const int32 theLockValue = CVarLocomotionData_LockTurnInPlace.GetValueOnAnyThread();
	if (theLockValue != INDEX_NONE)
	{
		theAngleDirectionIndex = theLockValue;
	}
#endif // WITH_EDITOR

	// If the desired index is invalid, we should use default index
	if (!TurnInPlaceParams.IsValidIndex(theAngleDirectionIndex) || !TurnInPlaceParams[theAngleDirectionIndex].IsValid())
	{
		bool bHasSubstituteData = false;

		// Check the substitute direction
		if (theAngleDirectionIndex == int32(EAngleDirection::BackwardLeft))
		{
			const int32 theSubstituteIndex = int32(EAngleDirection::BackwardLF);
			if (TurnInPlaceParams.IsValidIndex(theSubstituteIndex) && TurnInPlaceParams[theSubstituteIndex].IsValid())
			{
				bHasSubstituteData = true;
				theAngleDirectionIndex = theSubstituteIndex;
			}
		}
		else if (theAngleDirectionIndex == int32(EAngleDirection::BackwardRight))
		{
			const int32 theSubstituteIndex = int32(EAngleDirection::BackwardRF);
			if (TurnInPlaceParams.IsValidIndex(theSubstituteIndex) && TurnInPlaceParams[theSubstituteIndex].IsValid())
			{
				bHasSubstituteData = true;
				theAngleDirectionIndex = theSubstituteIndex;
			}
		}

		// If haven't substitute data, we will use default index
		if (bHasSubstituteData == false)
		{
			// Calculate the default direction index
			const int32 theDefaultDirectionIndex = (theAngleDirectionIndex <= int32(EAngleDirection::ForwardRF) || theAngleDirectionIndex % 2 == 0) ? 0 : 1;

			// Again check the default start params is valid
			if (TurnInPlaceParams.IsValidIndex(theDefaultDirectionIndex) && TurnInPlaceParams[theDefaultDirectionIndex].IsValid())
			{
				theAngleDirectionIndex = theDefaultDirectionIndex;
			}
			else
			{
				theAngleDirectionIndex = 0;
			}
		}
	}

	// Calculate the index range
	FIntPoint theRandomIndexRange = ResponseRandomTurnInPlaceIndexRange;

	// Check the desired gait axis is valid
	const FLocomotionGaitAttributesData* theGaitAttributesPtr = GetGaitAttributesData(InGaitAxisRange.X);
	if (theGaitAttributesPtr != nullptr)
	{
		// We always try get gait animation params
		const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange);
		if (theGaitAnimsPtr != nullptr && theGaitAnimsPtr->TurnAnimsIndicesRange.X >= 0)
		{
			theRandomIndexRange = theGaitAnimsPtr->TurnAnimsIndicesRange;
		}
		else if (theGaitAttributesPtr->RandomTurnInPlaceIndexRange.X >= 0)
		{
			theRandomIndexRange = theGaitAttributesPtr->RandomTurnInPlaceIndexRange;
		}
	}

	// Random the params type
	if (theRandomIndexRange.X != INDEX_NONE)
	{
		// Random the type index
		const int32 InType = FMath::RandRange(theRandomIndexRange.Y, theRandomIndexRange.Y);

		// Calculate the type params index
		const int32 theTypeParamsIndex = InType * int32(EAngleDirection::MAX) + theAngleDirectionIndex;

		// Check the type params is valid
		if (TurnInPlaceParams.IsValidIndex(theTypeParamsIndex) && TurnInPlaceParams[theTypeParamsIndex].IsValid())
		{
			return &TurnInPlaceParams[theTypeParamsIndex];
		}
	}

	// Return desired result
	return TurnInPlaceParams.IsValidIndex(theAngleDirectionIndex) ? &TurnInPlaceParams[theAngleDirectionIndex] : nullptr;
}
#pragma endregion


#pragma region Start
TAutoConsoleVariable<int32> CVarLocomotionData_LockStartStep(TEXT("a.AnimSkill.Locomotion.LockStartStep"), INDEX_NONE
	, TEXT("Lock the desired Params direction"));

const FLocomotionParams* UVMM_LocomotionData::GetStartParams(EAngleDirection InAngleDirection
	, ECardinalDirection InDirectionType, const FIntPoint& InGaitAxisRange, int32 InFootIndex) const
{
	// Check the desired gait axis is valid
	const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange);
	if (theGaitAnimsPtr == nullptr)
	{
		return nullptr;
	}

	// When the target is the forward direction, we intelligently select the starting direction according to the position of the foot
	if (InFootIndex != 0 && InDirectionType == ECardinalDirection::Forward && InAngleDirection <= EAngleDirection::ForwardRF)
	{
		InAngleDirection = InFootIndex < 0 ? EAngleDirection::ForwardLF : EAngleDirection::ForwardRF;
	}

	// If the move direction is not forward, we should always reset angle direction
	if (InDirectionType != ECardinalDirection::Forward)
	{
		InAngleDirection = EAngleDirection::MAX;
	}

	// Convert the angle direction type to int
	int32 theAngleDirectionIndex = int32(InAngleDirection);

#if WITH_EDITOR // Response lock event
	const int32 theLockValue = CVarLocomotionData_LockStartStep.GetValueOnAnyThread();
	if (theLockValue != INDEX_NONE)
	{
		theAngleDirectionIndex = theLockValue;
	}
#endif // WITH_EDITOR

	// Calculate desired direction index
	int32 theDirectionIndex = theAngleDirectionIndex + FMath::Max(0, int32(InDirectionType) - 1);

	// Get the desired gait start params
	const TArray<FLocomotionParams>& theStartParams = theGaitAnimsPtr->StartAnims;

	// Check the desired start params is valid
	if (!theStartParams.IsValidIndex(theDirectionIndex) || !theStartParams[theDirectionIndex].IsValid())
	{
		// Check the input data is valid
		if (InAngleDirection == EAngleDirection::MAX || InDirectionType != ECardinalDirection::Forward)
		{
			return nullptr;
		}

		// Check the substitute direction
		if (theAngleDirectionIndex == int32(EAngleDirection::BackwardLeft))
		{
			const int32 theSubstituteIndex = int32(EAngleDirection::BackwardLF);
			if (theStartParams.IsValidIndex(theSubstituteIndex) && theStartParams[theSubstituteIndex].IsValid())
			{
				return &theStartParams[theSubstituteIndex];
			}
		}
		else if (theAngleDirectionIndex == int32(EAngleDirection::BackwardRight))
		{
			const int32 theSubstituteIndex = int32(EAngleDirection::BackwardRF);
			if (theStartParams.IsValidIndex(theSubstituteIndex) && theStartParams[theSubstituteIndex].IsValid())
			{
				return &theStartParams[theSubstituteIndex];
			}
		}

		// Calculate the default direction index
		const int32 theDefaultDirectionIndex = (theAngleDirectionIndex <= int32(EAngleDirection::ForwardRF) || theAngleDirectionIndex % 2 == 0) ? 0 : 1;

		// Again check the default start params is valid
		if (theStartParams.IsValidIndex(theDefaultDirectionIndex) && theStartParams[theDefaultDirectionIndex].IsValid())
		{
			return &theStartParams[theDefaultDirectionIndex];
		}
		else if (theStartParams.IsValidIndex(0))
		{
			return &theStartParams[0];
		}

		// Failed
		return nullptr;
	}

	// Success
	return &theStartParams[theDirectionIndex];
}
#pragma endregion


#pragma region Moving
float UVMM_LocomotionData::GetRotationOffsetAlpha(const FIntPoint InGaitAxisRange, const ECardinalDirection InDirectionType) const
{
	// Check the desired gait axis is valid
	const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange);
	if (theGaitAnimsPtr == nullptr)
	{
		return 1.f;
	}

	// Calculate the data index
	const int32 theDataIndex = int32(InDirectionType);
	const int32 theDataNumber = theGaitAnimsPtr->RotationOffsetAlpha.Num();

	// Check the direction index is valid
	if (theGaitAnimsPtr->RotationOffsetAlpha.IsValidIndex(theDataIndex))
	{
		// return the result
		return theGaitAnimsPtr->RotationOffsetAlpha[theDataIndex];
	}

	// return the results
	return theDataNumber > 0 ? theGaitAnimsPtr->RotationOffsetAlpha[theDataNumber - 1] : 1.f;
}

UBlendSpace* UVMM_LocomotionData::GetRotationOffsetAsset(const FIntPoint InGaitAxisRange, const ECardinalDirection InDirectionType) const
{
	// Check the desired gait axis is valid
	const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange);
	if (theGaitAnimsPtr == nullptr)
	{
		return nullptr;
	}

	// Calculate the data index
	const int32 theDataIndex = int32(InDirectionType);
	const int32 theDataNumber = theGaitAnimsPtr->RotationOffsetAnims.Num();

	// Check the direction index is valid
	if (theGaitAnimsPtr->RotationOffsetAnims.IsValidIndex(theDataIndex))
	{
		// return the result
		return theGaitAnimsPtr->RotationOffsetAnims[theDataIndex];
	}

	// return the result
	return theDataNumber > 0 ? theGaitAnimsPtr->RotationOffsetAnims[theDataNumber - 1] : nullptr;
}

float UVMM_LocomotionData::GetCircleIncrementalAngle(const FIntPoint& InGaitAxisRange, const ERotationMode& InRotationMode, const ECardinalDirection& InDirectionType) const
{
	// Check the desired gait axis is valid
	const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange);
	if (theGaitAnimsPtr == nullptr)
	{
		return -1.f;
	}

	// Calculate the data index
	const int32 theDataIndex = InRotationMode == ERotationMode::Velocity ? int32(ECardinalDirection::MAX) : int32(InDirectionType);

	// Check the direction index is valid
	if (theGaitAnimsPtr->CircleIncrementalAngles.IsValidIndex(theDataIndex))
	{
		return theGaitAnimsPtr->CircleIncrementalAngles[theDataIndex];
	}

	// return the result
	return theGaitAnimsPtr->CircleIncrementalAngles.Num() > 0 ? theGaitAnimsPtr->CircleIncrementalAngles[0] : -1.f;
}

const FLocomotionParams* UVMM_LocomotionData::GetMovingParams(const FIntPoint& InGaitAxisRange, const ECardinalDirection& InDirectionType, int32 InAdditiveIndex) const
{
	// Check the desired gait axis is valid
	const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange);
	if (theGaitAnimsPtr == nullptr)
	{
		return nullptr;
	}

	// Calculate the direction index, each direction has corresponding 3(5) animations
	const int32 theDirectionIndex = int32(InDirectionType) * 3 + InAdditiveIndex;

	// Get the desired gait moving params
	const TArray<FLocomotionParams>& theMovingParams = theGaitAnimsPtr->MovingAnims;

	// Check the desired moving params is valid
	if (!theMovingParams.IsValidIndex(theDirectionIndex))
	{
		return nullptr;
	}

	// Success
	return &theMovingParams[theDirectionIndex];
}

const FLocomotionParams* UVMM_LocomotionData::GetMovingCircleParams(const FIntPoint& InGaitAxisRange, const ECardinalDirection& InDirectionType, const float& InArchDirection) const
{
	// Return desired result
	return GetMovingParams(InGaitAxisRange, InDirectionType, InArchDirection < 0.f ? 1 : 2);
}
#pragma endregion


#pragma region Pivot
TAutoConsoleVariable<int32> CVarLocomotionData_LockPivotStep(TEXT("a.AnimSkill.Locomotion.LockPivotStep"), INDEX_NONE
	, TEXT("Lock the desired Params direction"));

const FLocomotionParams* UVMM_LocomotionData::GetPivotParams(const EAngleDirection& InAngleDirection
, const ECardinalDirection& InDirectionType, const FIntPoint& InGaitAxisRange, const int32& InFootIndex) const
{
	// Check the foot index is valid
	if (InFootIndex == 0)
	{
		return nullptr;
	}

	// Check the desired gait axis is valid
	const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange);
	if (theGaitAnimsPtr == nullptr)
	{
		return nullptr;
	}

	// Calculate the direction index
	int32 theDirectionIndex = int32(InAngleDirection) * 2 + int32(InDirectionType) * 2 + FMath::Abs(InFootIndex) + (InFootIndex >= 0 ? 0 : -1);

#if WITH_EDITOR // Response lock event
	const int32 theLockValue = CVarLocomotionData_LockPivotStep.GetValueOnAnyThread();
	if (theLockValue != INDEX_NONE)
	{
		//theDirectionIndex = theLockValue * 2 + int32(InLastDirectionType) * 2 + FMath::Abs(InFootIndex) + (InFootIndex >= 0 ? 0 : -1);
		theDirectionIndex = theLockValue;
	}
#endif // WITH_EDITOR
	//theDirectionIndex = int32(EAngleDirection::Right) * 2 + int32(InLastDirectionType) * 2 + FMath::Abs(InFootIndex) + (InFootIndex >= 0 ? 0 : -1);
	//theDirectionIndex = int32(EAngleDirection::Right) * 2 + int32(InLastDirectionType) * 2;

	// Get the desired gait moving params
	const TArray<FLocomotionParams>& thePivotParams = theGaitAnimsPtr->PivotAnims;

	// Check the desired pivot params is valid
	if (!thePivotParams.IsValidIndex(theDirectionIndex))
	{
		return nullptr;
	}

	// Success
	return &thePivotParams[theDirectionIndex];
}

const bool UVMM_LocomotionData::CanResponsePivotStep(const int32 InGaitAxi) const
{
	// Check the global condition
	if (!bResponsePivotStep)
	{
		return false;
	}

	// Check the gait axis is valid
	if (InGaitAxi == INDEX_NONE)
	{
		return true;
	}

	// Check the desired gait axis is valid
	const FLocomotionGaitAttributesData* theGaitAttributesData = GetGaitAttributesData(InGaitAxi);
	if (theGaitAttributesData == nullptr)
	{
		return true;
	}

	// Return the result
	return theGaitAttributesData->bResponsePivot;
}

#pragma endregion


#pragma region Stop
const FLocomotionParams* UVMM_LocomotionData::GetStopParams(const FIntPoint& InGaitAxisRange, const int32& InFootIndex
	, const ECardinalDirection& InDirectionType, ELocomotionStopSection InStopType) const
{
	// Check the foot index is valid
	if (InFootIndex == 0)
	{
		return nullptr;
	}

	// Check the desired gait axis is valid
	const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange);
	if (theGaitAnimsPtr == nullptr)
	{
		return nullptr;
	}

	// Get the desired gait stop animation params
	const TArray<FLocomotionParams>& theStopParams = theGaitAnimsPtr->StopAnims;

	// If is transition type, we should use foot lock transition params
	if (InStopType == ELocomotionStopSection::MAX)
	{
		// Check the desired gait axis is valid
		const FLocomotionGaitAttributesData* theGaitAttributesPtr = GetGaitAttributesData(InGaitAxisRange.X);
		if (theGaitAttributesPtr != nullptr && !theGaitAttributesPtr->bResponseTransitionStop)
		{
			return nullptr;
		}

		if (const FLocomotionParams* theTransitionParams = GetTransitionParams(InFootIndex))
		{
			if (theTransitionParams->IsValid())
			{
				return theTransitionParams;
			}
		}

		// If is invalid params, we should down stop type.
		InStopType = ELocomotionStopSection(int32(InStopType) - 1);
	}

	// Calculate the stop type number
	const int32 theStopTypeNumber = FMath::Max(1, theStopParams.Num() / (int32(ECardinalDirection::MAX) * 2));

	// Calculate the desired direction index
	int32 theDirectionIndex = int32(InDirectionType) * 2 * theStopTypeNumber
		+ FMath::Abs(InFootIndex) + (InFootIndex >= 0 ? 0 : -1) + FMath::Min(int32(InStopType), (theStopTypeNumber - 1)) * 2;

	// Check the desired params is valid
	if (!theStopParams.IsValidIndex(theDirectionIndex))
	{
		// Failed
		return nullptr;
	}

	// Success
	return &theStopParams[theDirectionIndex];
}
#pragma endregion


#pragma region Jump
bool UVMM_LocomotionData::CanJump(const ERotationMode& InRotationMode)
{
	return true;
}

const FLocomotionParams* UVMM_LocomotionData::GetJumpParams(const ELocomotionStatus& InStatus
	, const FIntPoint& InGaitAxisRange, int32 InAxisIndex, int32 InDefaultAxisIndex) const
{
	// Local function
	auto OnGetJumpParams = [&](const TArray<FLocomotionJumpAnims>& InJumpParams, const ELocomotionStatus& InStatus
		, const FIntPoint& InGaitAxisRange, const int32& InAxisIndex, const int32& InDefaultAxisIndex)->const FLocomotionParams*
	{
		// Check the desired jump animation data is valid
		const FLocomotionJumpAnims* theJumpAnimsData = InJumpParams.IsValidIndex(InAxisIndex) ? &InJumpParams[InAxisIndex] : (InJumpParams.IsValidIndex(InDefaultAxisIndex) ? &InJumpParams[InDefaultAxisIndex] : nullptr);
		if (theJumpAnimsData == nullptr)
		{
			return nullptr;
		}

		// Return desired jump status params
		switch (InStatus)
		{
		case ELocomotionStatus::JUMPSTART:
			return &theJumpAnimsData->JumpStart;

		case ELocomotionStatus::JUMPFALLING:
			return theJumpAnimsData->JumpFallings.Num() > 0 ? &theJumpAnimsData->JumpFallings[0] : nullptr;

		case ELocomotionStatus::JUMPLAND:
			return &theJumpAnimsData->JumpLand;

		case ELocomotionStatus::JUMPMOBILELAND:
			return &theJumpAnimsData->JumpMobileLand;
		}

		// Failed
		return nullptr;
	};

	// We always check the mobile jump params is valid
	if (InGaitAxisRange.X != INDEX_NONE)
	{
		// Check the desired gait axis is valid
		if (const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(InGaitAxisRange))
		{
			// Check the mobile jump params is valid
			const FLocomotionParams* theMobileJumpParams = OnGetJumpParams(theGaitAnimsPtr->JumpAnims, InStatus, InGaitAxisRange, InAxisIndex, InDefaultAxisIndex);
			if (theMobileJumpParams != nullptr && theMobileJumpParams->IsValid())
			{
				return theMobileJumpParams;
			}
		}
	}

	// Return default in place jump params
	const FLocomotionParams* theInPlaceJumpParams = OnGetJumpParams(JumpInPlaceParams, InStatus, InGaitAxisRange, InAxisIndex, InDefaultAxisIndex);

	// Return result
	return theInPlaceJumpParams;
}
#pragma endregion


#pragma region Animation
const FLocomotionParams* UVMM_LocomotionData::GetIdleParams(const FIntPoint* InGaitAxisRange)
{
	// Random the params index
	int32 theRandomIndex = INDEX_NONE;

	// If the gait axis range is valid, we try get the gait idle params
	if (InGaitAxisRange != nullptr)
	{
		// Check the desired gait axis is valid
		const FLocomotionGaitAnims* theGaitAnimsPtr = GetGaitAnimsData(*InGaitAxisRange);
		if (theGaitAnimsPtr != nullptr && theGaitAnimsPtr->IdleAnimsIndicesRange.X >= 0)
		{
			theRandomIndex = FMath::RandRange(theGaitAnimsPtr->IdleAnimsIndicesRange.X, theGaitAnimsPtr->IdleAnimsIndicesRange.Y);
		}
	}

	// Check the gait params index is valid
	if (theRandomIndex == INDEX_NONE)
	{
		theRandomIndex = FMath::RandRange(0, IdlesParams.Num() - 1);
	}

	// Check the random params is valid
	if (IdlesParams.IsValidIndex(theRandomIndex) && IdlesParams[theRandomIndex].IsValid())
	{
		return &IdlesParams[theRandomIndex];
	}

	// Failed
	return nullptr;
}

const int32 UVMM_LocomotionData::GetValidGaitAnimsDataIndex(FIntPoint InGaitAxisRange, const ECardinalDirection& InCardinalDirection)
{
	// Check whether the maximum quantity limit is exceeded
	if (InGaitAxisRange.X > GaitParams.Num())
	{
		InGaitAxisRange.X = GaitParams.Num();
	}

	// While all gait axis
	while (InGaitAxisRange.X >= 0)
	{
		// Slow down gait axis
		--InGaitAxisRange.X;

		// Check the gait params is valid
		if (GaitParams.IsValidIndex(InGaitAxisRange.X))
		{
			// Check the desired gait attributes data is valid
			const FLocomotionGaitAttributesData* theGaitAttributesData = GetGaitAttributesData(InGaitAxisRange.X);
			if (theGaitAttributesData == nullptr)
			{
				continue;
			}

			// Check the gait support cardinal direction condition
			if (theGaitAttributesData->IgnoreCarinalDirections.Contains(InCardinalDirection))
			{
				continue;
			}

			// Success
			return InGaitAxisRange.X;
		}
	}

	// Failed
	return INDEX_NONE;
}

TAutoConsoleVariable<int32> CVarLocomotionData_LockGaitParamsIndex(TEXT("a.AnimSkill.Locomotion.LockGaitParamsIndex"), INDEX_NONE
	, TEXT("Lock the desired gait params index"));

const FLocomotionGaitAnims* UVMM_LocomotionData::GetGaitAnimsData(const FIntPoint& InGaitAxisRange) const
{
#if WITH_EDITOR // Response lock event
	const int32 theLockValue = CVarLocomotionData_LockGaitParamsIndex.GetValueOnAnyThread();
	if (theLockValue != INDEX_NONE)
	{
		// Check the gait range params is valid
		if (GaitAttributesData.IsValidIndex(InGaitAxisRange.X) && GaitParams.IsValidIndex(theLockValue))
		{
			return &GaitParams[theLockValue];
		}
	}
#endif // WITH_EDITOR

	// Check the gait range params is valid
	if (GaitAttributesData.IsValidIndex(InGaitAxisRange.X) && GaitParams.IsValidIndex(InGaitAxisRange.Y))
	{
		return &GaitParams[InGaitAxisRange.Y];
	}

#if 0
	// Check the base params index is valid
	const FLocomotionGaitAttributesData& theAttributesData = GaitAttributesData[InGaitAxisRange.X];
	if (theAttributesData.RandomIndexRange.X >= 0)
	{
		// Calculate the gait first params index
		const int32 theParamsIndex = theAttributesData.RandomIndexRange.X + InGaitAxisRange.Y;
		if (GaitParams.IsValidIndex(theParamsIndex))
		{
			return &GaitParams[theParamsIndex];
		}
	}
#endif

	// Return default result
	if (GaitParams.IsValidIndex(InGaitAxisRange.X))
	{
		return &GaitParams[InGaitAxisRange.X];
	}
	else if (GaitParams.IsValidIndex(GaitParams.Num() - 1))
	{
		return &GaitParams[GaitParams.Num() - 1];
	}

	// INVALID
	return nullptr;
}

const FLocomotionGaitAttributeData* UVMM_LocomotionData::GetGaitAttributeData(const uint8& InGaitAxis, const uint8& InLastGaitAxis) const
{
	// Check the attributes data is valid
	const FLocomotionGaitAttributesData* theAttributesData = GetGaitAttributesData(InLastGaitAxis);
	if (theAttributesData == nullptr)
	{
		return nullptr;
	}

	// Check the last gait to desired gait attributes is valid
	if (!theAttributesData->AttributesData.IsValidIndex(InGaitAxis))
	{
		return nullptr;
	}

	return &theAttributesData->AttributesData[InGaitAxis];
}

const FLocomotionGaitAttributesData* UVMM_LocomotionData::GetGaitAttributesData(const uint8& InGaitAxis) const
{
	// Check the desired gait attributes is valid
	if (!GaitAttributesData.IsValidIndex(InGaitAxis))
	{
		return nullptr;
	}

	// Return the result
	return &GaitAttributesData[InGaitAxis];
}
#pragma endregion


#pragma region Editor
class USkeleton* UVMM_LocomotionData::GetSkeleton(bool& bInvalidSkeletonIsError)
{
	bInvalidSkeletonIsError = false;

#if WITH_EDITORONLY_DATA
	if (Skeleton)
	{
		return Skeleton;
	}
	else if (PhysicsAsset && PhysicsAsset->GetPreviewMesh())
	{
		return PhysicsAsset->GetPreviewMesh()->GetSkeleton();
	}
#endif // WITH_EDITORONLY_DATA

	// INVALID
	return nullptr;
}

#if WITH_EDITOR
void UVMM_LocomotionData::ResetConfig()
{

}

void UVMM_LocomotionData::SortParamsIndex()
{
	uint8 Index = 0;
	LocomotionParams.Reset();

	OnSortLocomotionParamsIndex(Index, TurnInPlaceParams, ELocomotionStatus::TURNINPLACE);
	OnSortLocomotionParamsIndex(Index, IdlesParams, ELocomotionStatus::MAX);

	for (TPair<FName, FLocomotionTransitionAnims>& thePair : TransitionParamsMap)
	{
		for (FLocomotionParams& theAnims : thePair.Value.MultiParams)
		{
			OnSortLocomotionParamsIndex(Index, theAnims, ELocomotionStatus::TRANSITION);
		}
	}

	for (FLocomotionJumpAnims& theJumpAnims : JumpInPlaceParams)
	{
		OnSortLocomotionParamsIndex(Index, theJumpAnims.JumpStart, ELocomotionStatus::JUMPSTART, 0);
		OnSortLocomotionParamsIndex(Index, theJumpAnims.JumpFallings, ELocomotionStatus::JUMPFALLING, 0);
		OnSortLocomotionParamsIndex(Index, theJumpAnims.JumpLand, ELocomotionStatus::JUMPLAND, 0);
		OnSortLocomotionParamsIndex(Index, theJumpAnims.JumpMobileLand, ELocomotionStatus::JUMPMOBILELAND, 0);
	}

	for (int32 i = 0; i < GaitParams.Num(); i++)
	{
		FLocomotionGaitAnims& theGaitParams = GaitParams[i];
		OnSortLocomotionParamsIndex(Index, theGaitParams.StartAnims, ELocomotionStatus::START, i);
		OnSortLocomotionParamsIndex(Index, theGaitParams.MovingAnims, ELocomotionStatus::MOVING, i);
		OnSortLocomotionParamsIndex(Index, theGaitParams.StopAnims, ELocomotionStatus::STOP, i);
		OnSortLocomotionParamsIndex(Index, theGaitParams.PivotAnims, ELocomotionStatus::PIVOT, i);

		for (FLocomotionJumpAnims& theJumpAnims : theGaitParams.JumpAnims)
		{
			OnSortLocomotionParamsIndex(Index, theJumpAnims.JumpStart, ELocomotionStatus::JUMPSTART, i);
			OnSortLocomotionParamsIndex(Index, theJumpAnims.JumpLand, ELocomotionStatus::JUMPLAND, i);
			OnSortLocomotionParamsIndex(Index, theJumpAnims.JumpMobileLand, ELocomotionStatus::JUMPMOBILELAND, i);
		}
	}

	AnimationParamsNumber = Index + 1;

	PostEditChange();
	MarkPackageDirty();
}

void UVMM_LocomotionData::OnSortLocomotionParamsIndex(uint8& InIndex, TArray<FLocomotionParams>& InLocomotionParams, ELocomotionStatus InStatus, int32 InGaitAxisRange)
{
	FScopedSlowTask SortTask(InLocomotionParams.Num(), LOCTEXT("SortText", "Sort Params Index"));
	SortTask.MakeDialog(true);

	int32 theIndex = INDEX_NONE;
	for (FLocomotionParams& theLocomotionParams : InLocomotionParams)
	{
		++theIndex;
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (SortTask.ShouldCancel())
			break;

		SortTask.EnterProgressFrame();

		OnSortLocomotionParamsIndex(InIndex, theLocomotionParams, InStatus, InGaitAxisRange);

		if (InStatus == ELocomotionStatus::START)
		{
			//InLocomotionParams.LocomotionAngleDirection = EAngleDirection::Type(FMath::Min(int32(EAngleDirection::MAX), j));
			if (theIndex >= int32(EAngleDirection::MAX))
			{
				theLocomotionParams.CardinalDirection = ECardinalDirection(theIndex + 1 - int32(EAngleDirection::MAX));
			}
			else
			{
				theLocomotionParams.CardinalDirection = ECardinalDirection::Forward;
			}
		}
		else if (InStatus == ELocomotionStatus::STOP)
		{
			const int32 theCardinalDirectionIndex = theIndex / (InLocomotionParams.Num() / int32(ECardinalDirection::MAX));
			theLocomotionParams.CardinalDirection = ECardinalDirection(theCardinalDirectionIndex);
		}
	}
}

void UVMM_LocomotionData::OnSortLocomotionParamsIndex(uint8& InIndex, FLocomotionParams& InLocomotionParams, ELocomotionStatus InStatus, int32 InGaitAxisRange /*= 0*/)
{
	if (InLocomotionParams.IsValidAsset())
	{
		if (InStatus == ELocomotionStatus::MOVING)
		{
			InLocomotionParams.SyncGroup.GroupName = "Moving";
			InLocomotionParams.bLooping = true;
		}

		InLocomotionParams.Index = InIndex;
		InLocomotionParams.GaitAxis = uint8(InGaitAxisRange);
		InLocomotionParams.LocomotionStatus = InStatus;

		// Bake the animated rotation
		if (InLocomotionParams.Asset)
		{
			if (UVMM_Settings* theGlobalSettings = GetMutableDefault<UVMM_Settings>())
			{
				if (const FName* theCurveName = theGlobalSettings->GetMotionCurveName(EMotionCurveType::YawRotation))
				{
					TArray<float> theCurveTimes;
					TArray<float> theCurveValues;
					UAnimationBlueprintLibrary::GetFloatKeys(Cast<UAnimSequence>(InLocomotionParams.Asset), *theCurveName, theCurveTimes, theCurveValues);
					const float theMinValue = FMath::Min(theCurveValues);
					const float theMaxValue = FMath::Max(theCurveValues);
					InLocomotionParams.AnimatedRotation.Yaw = FMath::Abs(theMinValue) > theMaxValue ? theMinValue : theMaxValue;
				}
			}
		}
		if (InLocomotionParams.LocomotionStatus == ELocomotionStatus::PIVOT)
		{
			InLocomotionParams.BlendIn.SetBlendTime(0.2f);
			InLocomotionParams.BlendIn.SetBlendOption(EAlphaBlendOption::HermiteCubic);
			InLocomotionParams.BlendIn.Reset();
			InLocomotionParams.BlendOut.SetBlendTime(0.4f);
			InLocomotionParams.BlendOut.SetBlendOption(EAlphaBlendOption::HermiteCubic);
			InLocomotionParams.BlendOut.Reset();
		}

		LocomotionParams.Add(InLocomotionParams);
		++InIndex;
	}
	else
	{
		InLocomotionParams.Index = INDEX_NONE;
	}
}


#pragma region Bake Montage Slot
void UVMM_LocomotionData::BakeParamsMontageSlotsName()
{
	Modify();

	int32 theIndex = 0;
	for (TPair<FName, FLocomotionTransitionAnims>& thePair : TransitionParamsMap)
	{
		for (FLocomotionParams& theAnims : thePair.Value.MultiParams)
		{
			if (theIndex <= 1)
			{
				const int32* theDataIndex = MontageSlotDataIndexMap.Find(ELocomotionStatus::TURNINPLACE);
				if (theDataIndex != nullptr && MontageSlotsData.IsValidIndex(*theDataIndex))
				{
					theAnims.MontageSlotsName = MontageSlotsData[*theDataIndex].SlotsName;
				}
			}
			else
			{
				OnBakeLocomotionParamMontageSlotsName(theAnims);
			}
			++theIndex;
		}
	}
	OnBakeLocomotionParamsMontageSlotsName(TurnInPlaceParams);
	OnBakeLocomotionParamsMontageSlotsName(IdlesParams);

	for (FLocomotionJumpAnims& theJumpAnims : JumpInPlaceParams)
	{
		OnBakeLocomotionParamMontageSlotsName(theJumpAnims.JumpStart);
		OnBakeLocomotionParamsMontageSlotsName(theJumpAnims.JumpFallings);
		OnBakeLocomotionParamMontageSlotsName(theJumpAnims.JumpLand);
		OnBakeLocomotionParamMontageSlotsName(theJumpAnims.JumpMobileLand);
	}

	for (FLocomotionGaitAnims& theGaitParams : GaitParams)
	{
		OnBakeLocomotionParamsMontageSlotsName(theGaitParams.StartAnims);
		OnBakeLocomotionParamsMontageSlotsName(theGaitParams.MovingAnims);
		for (int Index = 0; Index < theGaitParams.MovingAnims.Num(); Index++)
		{
			// 0 3 6 9
			if (Index % 3 != 0)
			{
				theGaitParams.MovingAnims[Index].MontageSlotsName = GetMontageSlotName(ELocomotionStatus::MAX);
			}
		}
		OnBakeLocomotionParamsMontageSlotsName(theGaitParams.StopAnims);
		OnBakeLocomotionParamsMontageSlotsName(theGaitParams.PivotAnims);

		for (FLocomotionJumpAnims& theJumpAnims : theGaitParams.JumpAnims)
		{
			OnBakeLocomotionParamMontageSlotsName(theJumpAnims.JumpStart);
			OnBakeLocomotionParamMontageSlotsName(theJumpAnims.JumpLand);
			OnBakeLocomotionParamMontageSlotsName(theJumpAnims.JumpMobileLand);
		}
	}

	PostEditChange();
	MarkPackageDirty();
}

void UVMM_LocomotionData::OnBakeLocomotionParamsMontageSlotsName(TArray<FLocomotionParams>& InLocomotionDirectionParams)
{
	FScopedSlowTask BakeTask(InLocomotionDirectionParams.Num(), LOCTEXT("BakeText", "Bake Params MontageSlotsName"));
	BakeTask.MakeDialog(true);

	for (FLocomotionParams& theParams : InLocomotionDirectionParams)
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (BakeTask.ShouldCancel())
			break;

		BakeTask.EnterProgressFrame();
		OnBakeLocomotionParamMontageSlotsName(theParams);
	}
}

void UVMM_LocomotionData::OnBakeLocomotionParamMontageSlotsName(FLocomotionParams& InLocomotionDirectionParam)
{
	if (InLocomotionDirectionParam.IsValidAsset())
	{
		InLocomotionDirectionParam.MontageSlotsName = GetMontageSlotName(InLocomotionDirectionParam.LocomotionStatus);
	}
}

TArray<FName> UVMM_LocomotionData::GetMontageSlotName(const ELocomotionStatus InStatus)
{
	TArray<FName> theSlotsName;

	const int32* theDataIndex = MontageSlotDataIndexMap.Find(InStatus);
	if (theDataIndex != nullptr && MontageSlotsData.IsValidIndex(*theDataIndex))
	{
		theSlotsName = MontageSlotsData[*theDataIndex].SlotsName;
	}
	else if (MontageSlotsData.Num() > 0)
	{
		theSlotsName = MontageSlotsData.Last().SlotsName;
	}

	return theSlotsName;
}
#endif // WITH_EDITOR
#pragma endregion

#undef LOCTEXT_NAMESPACE