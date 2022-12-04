// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Librarys/VMM_PoseSearchLibrary.h"
#include "Librarys/VMM_CurveLibrary.h"
#include "VirtualMotionMatching.h"
#include "Curves/RichCurve.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimCompositeBase.h"
#include "Animation/AnimSequenceBase.h"

DECLARE_CYCLE_STAT(TEXT("Compare Poses"), STAT_MMComparePoses, STATGROUP_VirtualMotionMatching);
DECLARE_CYCLE_STAT(TEXT("Compare Poses Montage"), STAT_MMComparePosesMontage, STATGROUP_VirtualMotionMatching);
DECLARE_CYCLE_STAT(TEXT("Compare Poses Sequences"), STAT_MMComparePosesSequences, STATGROUP_VirtualMotionMatching);
DECLARE_CYCLE_STAT(TEXT("Calculate Previous Pose Distance"), STAT_MMCalculatePreviousPoseDistance, STATGROUP_VirtualMotionMatching);


bool UVMM_PoseSearchLibrary::ComparePoses(const float& InPreviousPoseDistance
	, const FVector2D& InLastCurveKey, const FVector2D& InCurrentCurveKey, float& OutPos, float& OutPosValue)
{
	SCOPE_CYCLE_COUNTER(STAT_MMComparePoses);

	// Ignore invalid key value
	if (InCurrentCurveKey.Y == 0.f || InLastCurveKey.Y == 0.f)
	{
		return false;
	}

	// Calculate the delta distance
	const float theDeltaDist = InPreviousPoseDistance - InCurrentCurveKey.Y;

	// Compare values ​​closer to 0
	if (FMath::Abs(OutPosValue) > FMath::Abs(theDeltaDist))
	{
		OutPosValue = theDeltaDist;
		OutPos = InCurrentCurveKey.X;

		// Fast path
		if (FMath::IsNearlyEqual(OutPosValue, 0.f, 0.1f))
		{
			return true;
		}
	}

	// Fast path, check the time range is valid
	if (InCurrentCurveKey.Y >= InLastCurveKey.Y)
	{
		if (FMath::IsWithinInclusive(InPreviousPoseDistance, InLastCurveKey.Y, InCurrentCurveKey.Y))
		{
			// Calculate the weight
			const float theValueRange = InCurrentCurveKey.Y - InLastCurveKey.Y;
			const float theWeight = theValueRange == 0.f ? 0.f : (InPreviousPoseDistance - InLastCurveKey.Y) / theValueRange;

			// Return the compare result
			OutPos = InLastCurveKey.X + (InCurrentCurveKey.X - InLastCurveKey.X) * theWeight;
			return true;
		}
	}
	else if (FMath::IsWithinInclusive(InPreviousPoseDistance, InCurrentCurveKey.Y, InLastCurveKey.Y))
	{
		// Calculate the weight
		const float theValueRange = InLastCurveKey.Y - InCurrentCurveKey.Y;
		const float theWeight = theValueRange == 0.f ? 0.f : (InPreviousPoseDistance - InCurrentCurveKey.Y) / theValueRange;

		// Return the compare result
		OutPos = InLastCurveKey.X + (InCurrentCurveKey.X - InLastCurveKey.X) * theWeight;
		return true;
	}

	// Return the compare result
	return false;
}

float UVMM_PoseSearchLibrary::ComparePoses(const float& InPreviousPoseDistance, const FRichCurve* InFuturePoseCurve)
{
	SCOPE_CYCLE_COUNTER(STAT_MMComparePoses);

	// Define the matching pose data
	float theMatchingPoseValue = 1e6f;
	float theMatchingPosePosition = -1.f;

	// Each every key
	for (int32 KeyIndex = 1; KeyIndex < InFuturePoseCurve->Keys.Num(); KeyIndex++)
	{
		const FRichCurveKey& theCurveKey = InFuturePoseCurve->Keys[KeyIndex];
		const FRichCurveKey& theCurveLastKey = InFuturePoseCurve->Keys[KeyIndex - 1];

		// Ignore invalid key value
		if (theCurveKey.Value == 0.f || theCurveLastKey.Value == 0.f)
		{
			continue;
		}

		// Calculate the delta distance
		const float theDeltaDist = InPreviousPoseDistance - theCurveKey.Value;

		// Compare values ​​closer to 0
		if (FMath::Abs(theMatchingPoseValue) > FMath::Abs(theDeltaDist))
		{
			theMatchingPoseValue = theDeltaDist;
			theMatchingPosePosition = theCurveKey.Time;

			// Fast path
			if (FMath::IsNearlyEqual(theMatchingPoseValue, 0.f, 0.1f))
			{
				return theMatchingPosePosition;
			}
		}

		// Fast path, check the time range is valid
		if (theCurveKey.Value >= theCurveLastKey.Value)
		{
			if (FMath::IsWithinInclusive(InPreviousPoseDistance, theCurveLastKey.Value, theCurveKey.Value))
			{
				// Calculate the weight
				const float theValueRange = theCurveKey.Value - theCurveLastKey.Value;
				const float theWeight = theValueRange == 0.f ? 0.f : (InPreviousPoseDistance - theCurveLastKey.Value) / theValueRange;

				// Return the compare result
				return theCurveLastKey.Time + (theCurveKey.Time - theCurveLastKey.Time) * theWeight;
			}
		}
		else if (FMath::IsWithinInclusive(InPreviousPoseDistance, theCurveKey.Value, theCurveLastKey.Value))
		{
			// Calculate the weight
			const float theValueRange = theCurveLastKey.Value - theCurveKey.Value;
			const float theWeight = theValueRange == 0.f ? 0.f : (InPreviousPoseDistance - theCurveKey.Value) / theValueRange;

			// Return the compare result
			return theCurveLastKey.Time + (theCurveKey.Time - theCurveLastKey.Time) * theWeight;
		}
	}

	// Return the compare result
	return theMatchingPosePosition;
}

float UVMM_PoseSearchLibrary::ComparePoses(const float& InEvaluatePos, const FRichCurve* InPreviousPoseCurve, const FRichCurve* InFuturePoseCurve)
{
	// Define the previous pose distance
	float thePreviousPoseDistance = 0.f;

	{
		SCOPE_CYCLE_COUNTER(STAT_MMCalculatePreviousPoseDistance);

		// Each every key
		for (int32 KeyIndex = 1; KeyIndex < InPreviousPoseCurve->Keys.Num(); KeyIndex++)
		{
			const FRichCurveKey& theCurveKey = InPreviousPoseCurve->Keys[KeyIndex];
			const FRichCurveKey& theCurveLastKey = InPreviousPoseCurve->Keys[KeyIndex - 1];

			// Ignore invalid key value
			if (theCurveKey.Value == 0.f || theCurveLastKey.Value == 0.f)
			{
				continue;
			}

			// Check the range
			if (FMath::IsWithinInclusive(InEvaluatePos, theCurveLastKey.Time, theCurveKey.Time))
			{
				// Calculate the weight
				const float theWeight = (InEvaluatePos - theCurveLastKey.Time) / (theCurveKey.Time - theCurveLastKey.Time);

				// Return the result
				thePreviousPoseDistance = theCurveLastKey.Value + (theCurveKey.Value - theCurveLastKey.Value) * theWeight;
				break;
			}
			else if (InEvaluatePos <= theCurveKey.Time)
			{
				thePreviousPoseDistance = theCurveKey.Value;
				break;
			}
		}

		// Check the pose distance is valid
		if (thePreviousPoseDistance == 0.f)
		{
			return 0.f;
		}
	}

	// Return compare result
	return ComparePoses(thePreviousPoseDistance, InFuturePoseCurve);
}

float UVMM_PoseSearchLibrary::ComparePosesAsMontage(float InEvaluatePos, UAnimMontage* InPreviousMontage, UAnimMontage* InFutureAnimMontage
	, const FName& InReferenceCurveName, const TMap<float, FName>& InPoseDistanceCurvesMap)
{
	//SCOPE_CYCLE_COUNTER(STAT_MMComparePosesMontage);

	// Check the animation montage is valid
	if (InPreviousMontage == nullptr || InPreviousMontage->SlotAnimTracks.Num() == 0)
	{
		return 0.f;
	}

	// Check the animation montage is valid
	if (InFutureAnimMontage == nullptr || InFutureAnimMontage->SlotAnimTracks.Num() == 0)
	{
		return 0.f;
	}

	// Find the previous animation asset
	UAnimSequenceBase* thePreviousAnimSequenceBase = nullptr;

	// Always evaluate first slot track
	for (const FAnimSegment& theAnimSegment : InPreviousMontage->SlotAnimTracks[0].AnimTrack.AnimSegments)
	{
		if (theAnimSegment.IsInRange(InEvaluatePos))
		{
			thePreviousAnimSequenceBase = theAnimSegment.AnimReference;
			break;
		}
	}

	// Check previous animation asset is valid
	if (thePreviousAnimSequenceBase == nullptr)
	{
		return 0.f;
	}

	// Find the evaluate animation asset
	float theCompositeStartPos = 0.f;
	UAnimSequenceBase* theEvalAnimSequenceBase = nullptr;

	// Always evaluate first slot track
	for (const FAnimSegment& theAnimSegment : InFutureAnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments)
	{
		theCompositeStartPos = theAnimSegment.AnimStartTime;
		theEvalAnimSequenceBase = theAnimSegment.AnimReference;
	}

	return ComparePosesAsAnimation(InEvaluatePos, Cast<UAnimSequence>(thePreviousAnimSequenceBase), Cast<UAnimSequence>(theEvalAnimSequenceBase), InReferenceCurveName, InPoseDistanceCurvesMap);
}

float UVMM_PoseSearchLibrary::ComparePosesAsAnimation(float InEvaluatePos, UAnimSequence* InPreviousAnimSequnece, UAnimSequence* InFutureAnimSequnece
	, const FName& InReferenceCurveName, const TMap<float, FName>& InPoseDistanceCurvesMap)
{
	SCOPE_CYCLE_COUNTER(STAT_MMComparePosesSequences);

	// Check the previous animation sequence is valid
	if (InPreviousAnimSequnece == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_PoseSearchLibrary::ComparePosesAsAnimation, Invalid previous animation."));
		return -1.f;
	}

	// Check the future animation sequence is valid
	if (InFutureAnimSequnece == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_PoseSearchLibrary::ComparePosesAsAnimation, Invalid previous animation."));
		return -1.f;
	}

	// Check the pose distance curves is valid
	if (InPoseDistanceCurvesMap.Num() == 0)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_PoseSearchLibrary::ComparePosesAsAnimation, Invalid pose distance curves."));
		return -1.f;
	}

	// Evaluate the reference curve value
	float theReferenceCurveValue = 0.f;
#if 0
	// Find the reference curve
	FRuntimeFloatCurve theReferenceCurve;
	if (!UVMM_CurveLibrary::ConvertAnimCurveToRuntimeCurve(InPreviousAnimSequnece, theReferenceCurve, InReferenceCurveName))
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_PoseSearchLibrary::ComparePosesAsAnimation, Invalid reference curve."));
		return -1.f;
	}

	const TArray<FRichCurveKey>& theReferenceKeys = theReferenceCurve.GetRichCurveConst()->Keys;
	for (int32 KeyIndex = 0; KeyIndex < theReferenceKeys.Num(); KeyIndex++)
	{
		const FRichCurveKey& theCurveKey = theReferenceKeys[KeyIndex];
		if (theReferenceKeys.IsValidIndex(KeyIndex + 1))
		{
			const FRichCurveKey& theNextCurveKey = theReferenceKeys[KeyIndex];
			if (InEvaluatePos >= theCurveKey.Time
				|| FMath::IsWithinInclusive(InEvaluatePos, theCurveKey.Time, theNextCurveKey.Time))
			{
				theReferenceCurveValue = theNextCurveKey.Value;
				break;
			}
		}
		else
		{
			theReferenceCurveValue = theCurveKey.Value;
			break;
		}
	}
#else
	// Check the curve data is valid
	bool bHasCurveData = false;
	theReferenceCurveValue = UVMM_CurveLibrary::GetAnimationAssetCurveValue(InPreviousAnimSequnece, InReferenceCurveName, InEvaluatePos, &bHasCurveData);
	if (!bHasCurveData)
	{
		return theReferenceCurveValue;
	}

#endif

#if 0
	// Find the desired pose distance curve
	for (const TPair<float, FName>& thePair : InPoseDistanceCurvesMap)
	{
		// Check the value is equal
		if (!FMath::IsNearlyEqual(thePair.Key, theReferenceCurveValue, 0.001f))
		{
			continue;
		}

		// Find the pose distance curve
		const FRichCurve* theFuturePoseDistanceCurve = &UVMM_CurveLibrary::GetFloatCurveClass<float, FFloatCurve>(InFutureAnimSequnece, thePair.Value)->FloatCurve;
		const FRichCurve* thePreviousPoseDistanceCurve = &UVMM_CurveLibrary::GetFloatCurveClass<float, FFloatCurve>(InPreviousAnimSequnece, thePair.Value)->FloatCurve;
		if (theFuturePoseDistanceCurve != nullptr && thePreviousPoseDistanceCurve != nullptr)
		{
			break;
		}

		// Check the pose distance is valid
		if (theFuturePoseDistanceCurve->IsEmpty() || thePreviousPoseDistanceCurve->IsEmpty())
		{
			return -1.f;
		}

		// Return the desired result
		return ComparePoses(InEvaluatePos, thePreviousPoseDistanceCurve, theFuturePoseDistanceCurve);
	}
#else
	// Find the desired pose distance curve
	for (const TPair<float, FName>& thePair : InPoseDistanceCurvesMap)
	{
		// Check the value is equal
		if (!FMath::IsNearlyEqual(thePair.Key, theReferenceCurveValue, 0.001f))
		{
			continue;
		}

		// Calculate the previous pose distance
		bool bHasPreviousCurveData = false;
		const float thePreviousPoseDistance = UVMM_CurveLibrary::GetAnimationAssetCurveValue(InPreviousAnimSequnece, thePair.Value, InEvaluatePos, &bHasPreviousCurveData);

		// Check the previous pose distance curve is valid
		if (bHasPreviousCurveData == false)
		{
			return -1.f;
		}

		// Define the pose distance data
		float theComparePosTime = -1.f;
		float theComparePosDistance = 1e6f;
		FVector2D theLastPoseKey(0.f, 0.f);

		// Get the animation asset data
		const int32& theNumberOfKeys = UVMM_CurveLibrary::GetNumberOfFrames(InFutureAnimSequnece);

		// Retrieve smart name for curve
		const FSmartName theCurveSmartName = UVMM_CurveLibrary::GetCurveSmartName(InFutureAnimSequnece, thePair.Value);

		// Each every pose keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Define the pose key 
			FVector2D thePoseKey(0.f, 0.f);

			// Evaluate the key time
			thePoseKey.X = UVMM_CurveLibrary::GetTimeAtFrame(InFutureAnimSequnece, KeyIndex);

			// Evaluate the pose distance
			thePoseKey.Y = InFutureAnimSequnece->EvaluateCurveData(theCurveSmartName.UID, thePoseKey.X);

			// Return the desired result
			if (ComparePoses(thePreviousPoseDistance, theLastPoseKey, thePoseKey, theComparePosTime, theComparePosDistance))
			{
				return theComparePosTime;
			}

			// Cache the last pose key
			theLastPoseKey = thePoseKey;
		}

		// Failed
		return theComparePosTime;
	}
#endif

	// Failed
	return -1.f;
}