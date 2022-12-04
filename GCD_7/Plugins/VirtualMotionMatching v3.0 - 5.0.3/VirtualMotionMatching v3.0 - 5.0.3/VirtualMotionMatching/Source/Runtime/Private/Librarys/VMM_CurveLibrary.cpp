// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Librarys/VMM_CurveLibrary.h"
#include "VirtualMotionMatching.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimCompositeBase.h"
#include "Components/SkeletalMeshComponent.h"

static bool RetrieveSmartNameForCurve(const UAnimSequence* InAnimSequence, const FName& InCurveName, const FName& InContainerName, FSmartName& OutSmartName)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));
	return InAnimSequence->GetSkeleton()->GetSmartNameByName(InContainerName, InCurveName, OutSmartName);
}

static FSmartName RetrieveSmartNameForCurve(const UAnimSequence* InAnimSequence, const FName& InCurveName, const FName& InContainerName)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));
	FSmartName SmartCurveName;
	InAnimSequence->GetSkeleton()->GetSmartNameByName(InContainerName, InCurveName, SmartCurveName);
	return SmartCurveName;
}

static FName RetrieveContainerNameForCurve(const UAnimSequence* InAnimSequence, const FName& InCurveName)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));

	{
		const FSmartNameMapping* theCurveMapping = InAnimSequence->GetSkeleton()->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
		if (theCurveMapping && theCurveMapping->Exists(InCurveName))
		{
			return USkeleton::AnimCurveMappingName;
		}
	}

	{
		const FSmartNameMapping* theCurveMapping = InAnimSequence->GetSkeleton()->GetSmartNameContainer(USkeleton::AnimTrackCurveMappingName);
		if (theCurveMapping && theCurveMapping->Exists(InCurveName))
		{
			return USkeleton::AnimTrackCurveMappingName;
		}
	}

	return NAME_None;
}

int32 UVMM_CurveLibrary::GetNumberOfKeys(UAnimSequence* InAnimSequence)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return INDEX_NONE;
	}

#if ENGINE_MAJOR_VERSION > 4
	return InAnimSequence->GetNumberOfSampledKeys();
#else
	return InAnimSequence->GetNumberOfFrames();
#endif
}

int32 UVMM_CurveLibrary::GetNumberOfFrames(UAnimSequence* InAnimSequence)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return INDEX_NONE;
	}

#if ENGINE_MAJOR_VERSION > 4
	return InAnimSequence->GetNumberOfSampledKeys();
#else
	return InAnimSequence->GetNumberOfFrames();
#endif
}

float UVMM_CurveLibrary::GetSequenceLength(UAnimSequenceBase* InAnimSequence)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return 0.f;
	}

#if ENGINE_MAJOR_VERSION > 4
	return InAnimSequence->GetPlayLength();
#else
	return InAnimSequence->GetPlayLength();
#endif
}

int32 UVMM_CurveLibrary::GetFrameAtTime(UAnimSequence* InAnimSequence, const float Time)
{
	check(InAnimSequence);
	const int32& theNumberOfKeys = UVMM_CurveLibrary::GetNumberOfFrames(InAnimSequence);
	const float& theAnimLength = UVMM_CurveLibrary::GetSequenceLength(InAnimSequence);
	const float FrameTime = theNumberOfKeys > 1 ? theAnimLength / (float)(theNumberOfKeys - 1) : 0.0f;
	return FMath::Clamp(FMath::RoundToInt(Time / FrameTime), 0, theNumberOfKeys - 1);
}

float UVMM_CurveLibrary::GetTimeAtFrame(UAnimSequence* InAnimSequence, const int32 Frame)
{
	check(InAnimSequence);
	const int32& theNumberOfKeys = UVMM_CurveLibrary::GetNumberOfFrames(InAnimSequence);
	const float& theAnimLength = UVMM_CurveLibrary::GetSequenceLength(InAnimSequence);
	const float FrameTime = theNumberOfKeys > 1 ? theAnimLength / (float)(theNumberOfKeys - 1) : 0.0f;
	return FMath::Clamp(FrameTime * Frame, 0.0f, theAnimLength);
}

FSmartName UVMM_CurveLibrary::GetCurveSmartName(UAnimSequence* InAnimSequence, const FName& InCurveName)
{
	check(InAnimSequence);

	// Get the curve container name from curve name
	const FName& theContainerName = RetrieveContainerNameForCurve(InAnimSequence, InCurveName);
	if (theContainerName == NAME_None)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_CurveLibrary::GetCurveSmartName, Invalid curve container name, curve name: %s."), *InCurveName.ToString());
		return FSmartName();
	}

	// Retrieve smart name for curve
	return RetrieveSmartNameForCurve(InAnimSequence, InCurveName, theContainerName);
}

template <typename DataType, typename CurveClass>
const CurveClass* UVMM_CurveLibrary::GetCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName, const ERawCurveTrackTypes& InCurveType)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));

	// Get the curve container name from curve name
	const FName& theContainerName = RetrieveContainerNameForCurve(InAnimSequence, InCurveName);
	if (theContainerName == NAME_None)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_CurveLibrary::GetCurveClass, Invalid curve container name, curve name: %s."), *InCurveName.ToString());
		return nullptr;
	}

	// Retrieve smart name for curve
	const FSmartName theCurveSmartName = RetrieveSmartNameForCurve(InAnimSequence, InCurveName, theContainerName);

	// Retrieve the curve by name
	return static_cast<const CurveClass*>(InAnimSequence->GetCurveData().GetCurveData(theCurveSmartName.UID, InCurveType));
}

template <typename DataType, typename CurveClass>
const FFloatCurve* UVMM_CurveLibrary::GetFloatCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName)
{
	// Check the curve class reference is valid
	const CurveClass* theCurvePtr = GetCurveClass<float, FFloatCurve>(InAnimSequence, InCurveName, ERawCurveTrackTypes::RCT_Float);
	if (theCurvePtr == nullptr)
	{
		//UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_CurveLibrary::GetFloatCurveClass, Invalid desired animation curve data, curve name: %s."), *InCurveName.ToString());
		return nullptr;
	}

	// Convert to float curve reference
	return static_cast<const FFloatCurve*>(theCurvePtr);
}

float UVMM_CurveLibrary::K2_EvaluateRuntimeCurve(float InEvaluatePos, const FRuntimeFloatCurve& InCurve)
{
	return InCurve.GetRichCurveConst()->Eval(InEvaluatePos);
}

void UVMM_CurveLibrary::K2_CalculateCurveRootMotion(float InDeltaSeconds, UAnimInstance* InAnimInstance, FTransform& OutRootMotion)
{
	// Check the animation instance is valid
	if (InAnimInstance == nullptr)
	{
		return;
	}
	
	// Check the owning mesh component is valid
	USkeletalMeshComponent* theMeshComponent = InAnimInstance->GetOwningComponent();
	if (theMeshComponent == nullptr)
	{
		return;
	}

	// Check the root motion transform is valid
	if (OutRootMotion.GetTranslation().IsNearlyZero() && OutRootMotion.GetRotation().IsIdentity())
	{
		return;
	}

	// Adjust the delta seconds multiply
	OutRootMotion.SetTranslation(OutRootMotion.GetTranslation() * InDeltaSeconds);
	OutRootMotion.SetRotation((OutRootMotion.Rotator() * InDeltaSeconds).Quaternion());

	// Convert to world space
	OutRootMotion = theMeshComponent->ConvertLocalRootMotionToWorld(OutRootMotion);
}

float UVMM_CurveLibrary::K2_GetCurveValueFromMontage(UAnimMontage* InAnimMontage, FName InCurveName, float InEvaluatePos, bool& OutHasCurve)
{
	return GetCurveValueFromMontage(InAnimMontage, InCurveName, InEvaluatePos, OutHasCurve);
}

float UVMM_CurveLibrary::GetCurveValueFromMontage(UAnimMontage* InAnimMontage, const FName& InCurveName, float InEvaluatePos, bool& OutHasCurve)
{
	// Check the animation montage is valid
	if (InAnimMontage == nullptr)
	{
		return 0.f;
	}

	// Check the slot track is valid
	if (InAnimMontage->SlotAnimTracks.Num() == 0)
	{
		return 0.f;
	}

	// Find the evaluate animation asset
	float theCompositeStartPos = 0.f;
	UAnimSequenceBase* theEvalAnimSequenceBase = nullptr;

	// Always evaluate first slot track
	for (const FAnimSegment& theAnimSegment : InAnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments)
	{
		if (theAnimSegment.IsInRange(InEvaluatePos))
		{
			theCompositeStartPos = theAnimSegment.StartPos;
			theEvalAnimSequenceBase = theAnimSegment.AnimReference;
			break;
		}
	}

	// Return the result
	return GetAnimationAssetCurveValue(Cast<UAnimSequence>(theEvalAnimSequenceBase), InCurveName, InEvaluatePos - theCompositeStartPos, &OutHasCurve);
}

float UVMM_CurveLibrary::K2_GetCurveValueFromSequence(UAnimSequence* InAnimSequence, FName InCurveName, float InEvaluatePos, bool& OutHasCurve)
{
	return GetAnimationAssetCurveValue(InAnimSequence, InCurveName, InEvaluatePos, &OutHasCurve);
}

float UVMM_CurveLibrary::GetAnimationAssetCurveValue(UAnimSequence* InAnimSequence, const FName& InCurveName, const float& InEvaluatePos, bool* OutHasCurve)
{
	// Always check the animation sequence is valid
	if (InAnimSequence == nullptr)
	{
		// Return the curve valid
		if (OutHasCurve != nullptr)
		{
			*OutHasCurve = false;
		}
		
		// Invalid
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_CurveLibrary::ConvertAnimCurveToRuntimeCurve, Invalid animation sequence asset."));
		return 0.f;
	}

#if WITH_EDITOR && 0
	// Check the curve data is valid
	const FFloatCurve* theFloatCurvePtr = GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, InCurveName);
	if (theFloatCurvePtr == nullptr || !theFloatCurvePtr->FloatCurve.HasAnyData())
	{
		// Return the curve valid
		if (OutHasCurve != nullptr)
		{
			*OutHasCurve = false;
		}

		// Invalid
		return 0.f;
	}

	// Get the keys reference
	const TArray<FRichCurveKey>& theCurveKeys = theFloatCurvePtr->FloatCurve.Keys;

	// Check the keys is valid
	if (theCurveKeys.Num() == 0)
	{
		// Return the curve valid
		if (OutHasCurve != nullptr)
		{
			*OutHasCurve = false;
		}
		return 0.f;
	}

	// Return the curve valid
	if (OutHasCurve != nullptr)
	{
		*OutHasCurve = true;
	}

	// Each every keys
	for (int32 KeyIndex = 1; KeyIndex < theCurveKeys.Num(); KeyIndex++)
	{
		const FRichCurveKey& keyA = theCurveKeys[KeyIndex - 1];
		const FRichCurveKey& keyB = theCurveKeys[KeyIndex];

		// Check value range
		if (FMath::IsNearlyEqual(InEvaluatePos, keyA.Time, KINDA_SMALL_NUMBER))
		{
			return keyA.Value;
		}
		else if (FMath::IsNearlyEqual(InEvaluatePos, keyB.Time, KINDA_SMALL_NUMBER))
		{
			return keyB.Value;
		}
		else if (FMath::IsWithinInclusive(InEvaluatePos, keyA.Time, keyB.Time))
		{
			// Calculate the weight
			const float theWeight = (InEvaluatePos - keyA.Time) / (keyB.Time - keyA.Time);

			// Return the result
			return keyA.Value + (keyB.Value - keyA.Value) * theWeight;
		}
	}

	// Return default value
	return theCurveKeys.Last().Value;
#else
	// Get the curve container name from curve name
	const FName& theContainerName = RetrieveContainerNameForCurve(InAnimSequence, InCurveName);
	if (theContainerName == NAME_None)
	{
		// Return the curve valid
		if (OutHasCurve != nullptr)
		{
			*OutHasCurve = false;
		}

		// Invalid
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_CurveLibrary::GetAnimationAssetCurveValue, Invalid curve container name, curve name: %s."), *InCurveName.ToString());
		return 0.f;
	}

	// Retrieve smart name for curve
	const FSmartName theCurveSmartName = RetrieveSmartNameForCurve(InAnimSequence, InCurveName, theContainerName);

	// Check the curve data is valid
	if (!InAnimSequence->HasCurveData(theCurveSmartName.UID, false))
	{
		return 0.f;
	}

	// Flag the curve is valid
	if (OutHasCurve != nullptr)
	{
		*OutHasCurve = true;
	}

	// Evaluate the curve data
	const float theCurveValue = InAnimSequence->EvaluateCurveData(theCurveSmartName.UID, InEvaluatePos);

	// Return the result
	return theCurveValue;
#endif
}