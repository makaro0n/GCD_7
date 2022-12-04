// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VMM_PoseSearchLibrary.generated.h"

class UAnimMontage;
class UAnimSequence;
struct FRichCurve;

/**
 *
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_PoseSearchLibrary : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Evaluate pose comparison metric between a pose in the search pose distance and an input query
	*
	* @param InPreviousPoseDistance		Previous animation pose distance
	* @param InLastCurveKey			The animation pose of the next curve last key, referring to the next animation pose
	* @param InCurrentCurveKey			The animation pose of the next curve current key, referring to the next animation pose
	* @return OutPos			Return best pose time
	* @return OutPosValue			Return best pose delta distance value
	*
	* @return Dissimilarity between the two poses
	*/
	static bool ComparePoses(const float& InPreviousPoseDistance, const FVector2D& InLastCurveKey, const FVector2D& InCurrentCurveKey
		, float& OutPos, float& OutPosValue);

	/**
	* Evaluate pose comparison metric between a pose in the search pose distance and an input query
	*
	* @param InPreviousPoseDistance		Previous animation pose distance
	* @param InFuturePoseCurve			The animation pose of the next curve, referring to the next animation pose
	*
	* @return Dissimilarity between the two poses
	*/
	static float ComparePoses(const float& InPreviousPoseDistance, const FRichCurve* InFuturePoseCurve);

	/**
	* Evaluate pose comparison metric between a pose in the search pose distance and an input query
	*
	* @param InEvaluatePos	The animation pose of the current query, referring to the previous animation pose
	* @param InPreviousPoseCurve		The animation pose of the current curve, referring to the previous animation pose
	* @param InFuturePoseCurve			The animation pose of the next curve, referring to the next animation pose
	*
	* @return Dissimilarity between the two poses
	*/
	static float ComparePoses(const float& InEvaluatePos, const FRichCurve* InPreviousPoseCurve, const FRichCurve* InFuturePoseCurve);

	/**
	* Evaluate pose comparison metric between a pose in the search pose distance and an input query
	*
	* @param InEvaluatePos	      The animation pose of the current query, referring to the previous animation pose
	* @param InPreviousMontage		The previous animation montage
	* @param InFuturePoseCurve			The next animation montage
	*
	* @return Dissimilarity between the two poses
	*/
	UFUNCTION(BlueprintPure, Category = "VMM| PoseSearch")
	static float ComparePosesAsMontage(float InEvaluatePos, UAnimMontage* InPreviousMontage, UAnimMontage* InFutureAnimMontage
			, const FName& InReferenceCurveName, const TMap<float, FName>& InPoseDistanceCurvesMap);

	/**
	* Evaluate pose comparison metric between a pose in the search pose distance and an input query
	*
	* @param InEvaluatePos	     The animation pose of the current query, referring to the previous animation pose
	* @param InPreviousAnimSequnece		The previous animation sequence
	* @param InFutureAnimSequnece			The next animation sequence
	*
	* @return Dissimilarity between the two poses
	*/
	UFUNCTION(BlueprintPure, Category = "VMM| PoseSearch")
	static float ComparePosesAsAnimation(float InEvaluatePos, UAnimSequence* InPreviousAnimSequnece, UAnimSequence* InFutureAnimSequnece
		, const FName& InReferenceCurveName, const TMap<float, FName>& InPoseDistanceCurvesMap);
};