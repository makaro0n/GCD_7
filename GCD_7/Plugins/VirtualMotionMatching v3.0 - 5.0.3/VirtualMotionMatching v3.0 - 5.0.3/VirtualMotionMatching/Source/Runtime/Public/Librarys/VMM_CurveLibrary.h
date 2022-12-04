// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Animation/SmartName.h"
#include "Curves/RealCurve.h"
#include "Animation/AnimCurveTypes.h"
#include "Curves/CurveFloat.h"
#include "VMM_CurveLibrary.generated.h"

class UAnimMontage;
class UAnimInstance;
class UAnimSequence;
struct FFloatCurve;

/**
 *
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_CurveLibrary : public UObject
{
	GENERATED_BODY()

public:

    /** Get the time at the frame */
	static float GetFrameTime(UAnimSequenceBase* InAnimSequence);

	/** Get the keys number at the animation asset */
	static int32 GetNumberOfKeys(UAnimSequence* InAnimSequence);

	/** Get the frames number at the animation asset */
	static int32 GetNumberOfFrames(UAnimSequence* InAnimSequence);

	/** Get the sequence length at the animation asset */
	static float GetSequenceLength(UAnimSequenceBase* InAnimSequence);

public:

	/** Get the frame number for the provided time */
	static int32 GetFrameAtTime(UAnimSequence* InAnimSequence, const float Time);

	/** Get the time at the given frame */
	static float GetTimeAtFrame(UAnimSequence* InAnimSequence, const int32 Frame);

	/** Return the curve smart name from animation sequence */
	static FSmartName GetCurveSmartName(UAnimSequence* InAnimSequence, const FName& InCurveName);

	/** Return desired curve class */
	template <typename DataType, typename CurveClass>
	static const CurveClass* GetCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName, const ERawCurveTrackTypes& InCurveType);

	/** Return float curve class */
	template <typename DataType, typename CurveClass>
	static const FFloatCurve* GetFloatCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName);

public:

	/**
	 * Evaluate runtime float curve value.
	 * @param	InEvaluatePos	  Evaluate pos
	 * @param	InEvaluatePos	  Evaluate pos
	 * @return  Curve value
	 */
	UFUNCTION(BlueprintCallable, Category = "VMM| Curve", meta = (DisplayName = "Evaluate Runtime Curve"))
	static float K2_EvaluateRuntimeCurve(float InEvaluatePos, const FRuntimeFloatCurve& InCurve);

	/**
	 * Calculate root motion from animation curves.
	 * @param	InDeltaSeconds	  World delta seconds
	 * @param	InAnimInstance			Animation instance reference
	 * @param	OutRootMotion			Convert local space root motion to world space root motion
	 * @return  Root motion world transform
	 */
	UFUNCTION(BlueprintCallable, Category = "VMM| Curve", meta = (DisplayName = "Calculate Curve Root Motion"))
	static void K2_CalculateCurveRootMotion(float InDeltaSeconds, UAnimInstance* InAnimInstance, UPARAM(ref) FTransform& OutRootMotion);

	/**
	 * Get curve value in desired animation montage position.
	 * @param	InAnimMontage	The specified animation montage
	 * @param	InCurveName			The specified animation curve name
	 * @param	InEvaluatePos			The specified animation curve position
	 * @param	OutHasCurve			Return has curve condition
	 * @return  Curve value
	 */
	UFUNCTION(BlueprintPure, Category = "VMM| Curve", meta = (DisplayName = "Get Curve Value From Montage"))
	static float K2_GetCurveValueFromMontage(UAnimMontage* InAnimMontage, FName InCurveName, float InEvaluatePos, bool& OutHasCurve);
	static float GetCurveValueFromMontage(UAnimMontage* InAnimMontage, const FName& InCurveName, float InEvaluatePos, bool& OutHasCurve);

	/**
	 * Get curve value in desired animation sequence position.
	 * @param	InAnimSequence	The specified animation sequence
	 * @param	InCurveName			The specified animation curve name
	 * @param	InEvaluatePos			The specified animation curve position
	 * @param	OutHasCurve			Return has curve condition
	 * @return  Curve value
	 */
	UFUNCTION(BlueprintPure, Category = "VMM| Curve", meta = (DisplayName = "Get Curve Value From Sequence"))
	static float K2_GetCurveValueFromSequence(UAnimSequence* InAnimSequence, FName InCurveName, float InEvaluatePos, bool& OutHasCurve);
	
	/** Return animation curve value in desired position */
	static float GetAnimationAssetCurveValue(UAnimSequence* InAnimSequence, const FName& InCurveName, const float& InEvaluatePos, bool* OutHasCurve = nullptr);
};