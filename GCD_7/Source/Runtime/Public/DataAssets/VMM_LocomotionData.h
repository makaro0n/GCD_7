// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VMM_Types.h"
#include "Interfaces/Interface_BoneReferenceSkeletonProvider.h"
#include "VMM_LocomotionData.generated.h"

class USkeleton;
class UPhysicsAsset;
class UAnimSequence;
class UBlendSpace;

/**
 * Data asset dedicated to managing animation motion params comes from Virtual Motion Matching
 */
UCLASS(Blueprintable)
class VIRTUALMOTIONMATCHING_API UVMM_LocomotionData : public UDataAsset, public IBoneReferenceSkeletonProvider
{
	GENERATED_UCLASS_BODY()

#if 1 // Virtual Animation Skills

protected:

	/*-----------------------------------------------------------------------------
		It comes from the virtual animation skills plug-in.
		Once the two plug-ins need to be combined, we will focus on VAS.
	-----------------------------------------------------------------------------*/

	/** Fast path data name in data manager - Should Register Data Registry */
	UPROPERTY(EditDefaultsOnly, Category = Performance)
	FName DataName;

	/** Fast path data index in data manager */
	UPROPERTY(VisibleAnywhere, Category = Performance)
	uint16 DataIndex;

public:

	/** Set the global data name from global manager */
	void SetDataName(const FName& InDataName) { DataName = InDataName; }

	/** Set the global data index from global manager */
	void SetDataIndex(const uint16 InDataIndex) { DataIndex = InDataIndex; }

	/** Return fast path data name from data manager */
	FORCEINLINE const FName& GetDataName() const { return DataName; }

	/** Return fast path data index from data manager */
	FORCEINLINE const uint16& GetDataIndex() const { return DataIndex; }

#endif // Virtual Animation Skills


#pragma region Pose Search

public:

	/** Use pose matching to choose the foot weight curve name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PoseSearch)
	FName FootWeightCurveName;

	/** Use pose matching to choose the foot position curve name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PoseSearch)
	FName FootPositionCurveName;

	/** Use pose matching to choose the pose distance curves name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PoseSearch)
	TMap<float, FName> PoseDistanceCurvesNameMap;

public:

	/** Return the foot weight curve name */
	FORCEINLINE const FName& GetFootWeightCurveName() const { return FootWeightCurveName; }

	/** Return the foot position curve name */
	FORCEINLINE const FName& GetFootPositionCurveName() const { return FootPositionCurveName; }

	/** Return the pose distance curves name map */
	FORCEINLINE const TMap<float, FName>& GetPoseDistanceCurvesNameMap() const { return PoseDistanceCurvesNameMap; }

#pragma endregion

#if WITH_EDITOR
	/**
	 * This alternate version of PostEditChange is called when properties inside structs are modified.  The property that was actually modified
	 * is located at the tail of the list.  The head of the list of the FStructProperty member variable that contains the property that was modified.
	 */
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent) override;
#endif // WITH_EDITOR

#pragma region Aim Offset

protected:

	/** AimOffset animation asset */
	UPROPERTY(EditDefaultsOnly, Category = AimOffset)
	UBlendSpace* AimOffset;

	/** Important bone names for baking */
	UPROPERTY(VisibleDefaultsOnly, Category = AimOffset)
	TArray<FName> PhysicsBonesName;

public:

	/** Return the aim offset asset */
	FORCEINLINE UBlendSpace* GetAimOffsetAsset() const { return AimOffset; }

	/** Return the aim offset asset physics bone name */
	FORCEINLINE const TArray<FName>& GetPhysicsBonesName() const { return PhysicsBonesName; }

#pragma endregion


#pragma region Transition

protected:

	/** The transition animation type will be randomly selected based on this range */
	UPROPERTY(EditDefaultsOnly, Category = Transition)
	FIntPoint ResponseRandomTransitionIndexRange;

public:

	/** Transition animation params */
	UPROPERTY(EditDefaultsOnly, Category = Transition)
	TMap<FName, FLocomotionTransitionAnims> TransitionParamsMap;

public:

	/** Return transition params from input data. */
	const FLocomotionParams* GetTransitionParams(const int32& InFootIndex, const FName* InName = nullptr) const;

	/** Return transition params from input data. */
	const FLocomotionParams* GetTransitionParams(const FLocomotionTransitionAnims* theAnimParams, const int32 InTypeIndex = 0) const;

	/** Return transition animation params from input data. */
	const FLocomotionTransitionAnims* GetTransitionAnimParams(const int32& InFootIndex, const FName* InName = nullptr) const;

#pragma endregion


#pragma region TurnInPlace

protected:

	/** Defines whether to perform a turn animation in response */
	UPROPERTY(EditDefaultsOnly, Category = TurnInPlace)
	uint8 bResponseTurnInPlace : 1;

	/** Minimum interval time for responding to turn animations */
	UPROPERTY(EditDefaultsOnly, Category = TurnInPlace)
	float ResponseTurnInPlaceIntervalTime;

	/** Minimum angle for responsive turn animation */
	UPROPERTY(EditDefaultsOnly, Category = TurnInPlace)
	FVector2D ResponseTurnInPlaceAngleRange;

	/** The turn animation type will be randomly selected based on this range, in units of 10 */
	UPROPERTY(EditDefaultsOnly, Category = TurnInPlace/*, meta = (UIMin = "0", ClampMin = "0")*/)
	FIntPoint ResponseRandomTurnInPlaceIndexRange;

	/** Turn in place animation asset params */
	UPROPERTY(EditDefaultsOnly, Category = TurnInPlace)
	TArray<FLocomotionParams> TurnInPlaceParams;

public:

	/** Return turn in place params from desired angle direction. */
	const FLocomotionParams* GetTurnInPlaceParams(const FIntPoint& InGaitAxisRange, const EAngleDirection& InAngleDirection) const;

public:

	/** Return the response turn in place animation condition */
	FORCEINLINE const bool CanResponseTurnInPlace() const { return bResponseTurnInPlace; }

	/** Return the response turn in place animation minimum angle */
	FORCEINLINE const FVector2D& GetResponseTurnInPlaceAngleRange() const { return ResponseTurnInPlaceAngleRange; }

	/** Return the response turn in place animation minimum interval time */
	FORCEINLINE const float& GetResponseTurnInPlaceIntervalTime() const { return ResponseTurnInPlaceIntervalTime; }

#pragma endregion


#pragma region Start

protected:

	/** Defines whether to perform a start animation in response */
	UPROPERTY(EditDefaultsOnly, Category = Start)
	uint8 bResponseStartStep : 1;

	/** If true, do not respond when the TurnInPlace end */
	UPROPERTY(EditDefaultsOnly, Category = Start)
	uint8 bResponseStartStepWaitTurn : 1;

	/** If true, do not respond when the foot is locked */
	UPROPERTY(EditDefaultsOnly, Category = Start)
	uint8 bResponseStartStepWaitFootLock : 1;

	/** Delay time to respond to the turn animation and then respond to the input */
	UPROPERTY(EditDefaultsOnly, Category = Start)
	float ResponseStartAfterTurnTime;

	/** Delay time to respond to the start animation from stop status */
	UPROPERTY(EditDefaultsOnly, Category = Start)
	float ResponseStartAfterStopTime;

public:

	/** Return start animation params from desired data. */
	const FLocomotionParams* GetStartParams(EAngleDirection InAngleDirection, ECardinalDirection InDirectionType, const FIntPoint& InGaitAxisRange, int32 InFootIndex = 0) const;

public:

	/** Return the response start step animation condition */
	FORCEINLINE const bool CanResponseStartStep() const { return bResponseStartStep; }

	/** Return the response start step animation condition */
	FORCEINLINE const bool CanResponseStartStepWaitTurn() const { return bResponseStartStepWaitTurn; }

	/** Return the response start step animation condition */
	FORCEINLINE const bool CanResponseStartStepWaitFootLock() const { return bResponseStartStepWaitFootLock; }

	/** Return delay time to respond to the turn animation and then respond to the input */
	FORCEINLINE const float& GetResponseStartAfterTurnTime() const { return ResponseStartAfterTurnTime; }

	/** Return delay time to respond to the start animation and then respond to the input */
	FORCEINLINE const float& GetResponseStartAfterStopTime() const { return ResponseStartAfterStopTime; }

#pragma endregion


#pragma region Moving

protected:

	/** Defines whether to perform a moving arch animation in response */
	UPROPERTY(EditDefaultsOnly, Category = Moving)
	uint8 bResponseCircle : 1;

	/** If it is true, we also use moving circle in looking rotation mode */
	UPROPERTY(EditDefaultsOnly, Category = Moving)
	uint8 bResponseCircleInLookingMode : 1;

	/** Defines whether to perform a moving arch animation in response */
	UPROPERTY(EditDefaultsOnly, Category = Moving)
	float ResponseCicleAngle;

public:

	/** Return rotation offset alpha from gait axis. */
	float GetRotationOffsetAlpha(const FIntPoint InGaitAxisRange = FIntPoint(0), const ECardinalDirection InDirectionType = ECardinalDirection::Forward) const;

	/** Return movement lean animation blend space from gait axis. */
	UBlendSpace* GetRotationOffsetAsset(const FIntPoint InGaitAxisRange = FIntPoint(0), const ECardinalDirection InDirectionType = ECardinalDirection::Forward) const;

	/** Return moving circle incremental angle from input data */
	float GetCircleIncrementalAngle(const FIntPoint& InGaitAxisRange, const ERotationMode& InRotationMode, const ECardinalDirection& InDirectionType) const;

	/** Return moving animation params from input data. */
	const FLocomotionParams* GetMovingParams(const FIntPoint& InGaitAxisRange, const ECardinalDirection& InDirectionType, int32 InAdditiveIndex = 0) const;

	/** Return moving circle animation params from input data. */
	const FLocomotionParams* GetMovingCircleParams(const FIntPoint& InGaitAxisRange, const ECardinalDirection& InDirectionType, const float& InArchDirection) const;

public:

	/** Return the response moving arch animation condition */
	FORCEINLINE const bool CanResponseCircle() const { return bResponseCircle; }

	/** Return use moving circle in looking rotation mode condition */
	FORCEINLINE const bool CanbResponseCircleInLookingMode() const { return bResponseCircleInLookingMode; }

	/** Return the response moving arch animation angle */
	FORCEINLINE const float& GetResponseCicleAngle() const { return ResponseCicleAngle; }

#pragma endregion


#pragma region Pivot

protected:

	/** Defines whether to perform a pivot animation in response */
	UPROPERTY(EditDefaultsOnly, Category = Pivot)
	uint8 bResponsePivotStep : 1;

	/** If it is true, we will respond to stop first in the smart pivot, otherwise we will respond to start directly */
	UPROPERTY(EditDefaultsOnly, Category = Pivot)
	uint8 bResponsePivotStop : 1;

	/** Minimum angle for responding to pivot animations */
	UPROPERTY(EditDefaultsOnly, Category = Pivot)
	float ResponsePivotAngle;

	/** Minimum interval for responding to pivot animations */
	UPROPERTY(EditDefaultsOnly, Category = Pivot)
	float ResponsePivotIntervalTime;

public:
	/** Stop to pivot condition - moving montage weight: MontageWeight >= this */
	UPROPERTY(EditDefaultsOnly, Category = Pivot)
	float ResponsePivotAsMovingWeightFromStop;

	/** Stop to pivot condition - moving montage blend out time: BlendOutTime <= this */
	UPROPERTY(EditDefaultsOnly, Category = Pivot)
	float ResponsePivotAsMovingBlendTimeFromStop;

public:

	/** Return pivot animation params from input data. */
	const FLocomotionParams* GetPivotParams(const EAngleDirection& InAngleDirection
		, const ECardinalDirection& InDirectionType, const FIntPoint& InGaitAxisRange, const int32& InFootIndex) const;

public:

	/** Return the response pivot animation condition */
	const bool CanResponsePivotStep(const int32 InGaitAxis = INDEX_NONE) const;

	/** Return the response pivot stop animation condition */
	FORCEINLINE const bool CanResponsePivotStop() const { return bResponsePivotStop; }

	/** Return the response pivot animation minimum angle */
	FORCEINLINE const float& GetResponsePivotAngle() const { return ResponsePivotAngle; }

	/** Return the response pivot animation minimum interval time */
	FORCEINLINE const float& GetResponsePivotIntervalTime() const { return ResponsePivotIntervalTime; }

#pragma endregion


#pragma region Stop

protected:

	/** Defines whether to perform a stop animation in response */
	UPROPERTY(EditDefaultsOnly, Category = Stop)
	uint8 bResponseStopStep : 1;

public:

	/** Return stop animation params from input data. */
	const FLocomotionParams* GetStopParams(const FIntPoint& InGaitAxisRange, const int32& InFootIndex
		, const ECardinalDirection& InDirectionType, ELocomotionStopSection InStopType = ELocomotionStopSection::Normal) const;

public:

	/** Return the response pivot animation condition */
	FORCEINLINE const bool CanResponseStopStep() const { return bResponseStopStep; }

#pragma endregion


#pragma region Jump

protected:

	/** The default global jump height */
	UPROPERTY(EditDefaultsOnly, Category = Jump)
	float GlobalJumpHeight;

	/** Jump in place animation, we can customize the index */
	UPROPERTY(EditDefaultsOnly, Category = Jump)
	TArray<FLocomotionJumpAnims> JumpInPlaceParams;

public:

	/** Return whether it is possible to jump */
	bool CanJump(const ERotationMode& InRotationMode);

	/** Return Jump params from input data. */
	const FLocomotionParams* GetJumpParams(const ELocomotionStatus& InStatus, const FIntPoint& InGaitAxisRange
		, int32 InAxisIndex = 0, int32 InDefaultAxisIndex = 0) const;

public:

	/** Return the global jump height */
	FORCEINLINE const float& GetGlobalJumpHeight() const { return GlobalJumpHeight; }

#pragma endregion


#pragma region Animation

	/** Base pose animation. */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimSequence* BasePose;

	/** Idle animations. */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	TArray<FLocomotionParams> IdlesParams;

	/** Gait animation in place, we can customize the index, Start, Moving, Stop, Pivot, Jump etc.. */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	TArray<FLocomotionGaitAnims> GaitParams;

	/** Animation index set corresponding to each gait can be a gait corresponding to multiple animation types */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	TArray<FLocomotionGaitAttributesData> GaitAttributesData;

	/** Number of animations currently collected */
	UPROPERTY(VisibleDefaultsOnly, Category = Animation)
	int32 AnimationParamsNumber;

	/*
	 * Collect all animation params so that you can quickly get the specified params according to the index
	 * Not displayed in the editor, because it may cause lag
	 */
	UPROPERTY()
	TArray<FLocomotionParams> LocomotionParams;
	//TArray<FLocomotionParams, TInlineAllocator<256>> LocomotionParams;

public:

	// ------------------------------------------ Idle ------------------------------------------ //
	/** Return the idle params from animations array. */
	const FLocomotionParams* GetIdleParams(const FIntPoint* InGaitAxisRange = nullptr);

	// ------------------------------------------ Gait ------------------------------------------ //

	/** Return valid gait animation data index from input data */
	const int32 GetValidGaitAnimsDataIndex(FIntPoint InGaitAxisRange, const ECardinalDirection& InCardinalDirection);

	/** Return gait animation data from gait axis */
	const FLocomotionGaitAnims* GetGaitAnimsData(const FIntPoint& InGaitAxisRange) const;

	/** Return gait attribute data from gait axis */
	const FLocomotionGaitAttributeData* GetGaitAttributeData(const uint8& InGaitAxis, const uint8& InLastGaitAxis) const;

	/** Return gait attributes data from gait axis */
	const FLocomotionGaitAttributesData* GetGaitAttributesData(const uint8& InGaitAxis) const;

	// ------------------------------------------ Params ------------------------------------------ //

	/** Return the locomotion params from animations array. */
	FORCEINLINE FLocomotionParams* GetLocomotionParamsByIndex(const uint8& InParamsIndex)
	{
		return LocomotionParams.IsValidIndex(InParamsIndex) ? &LocomotionParams[InParamsIndex] : nullptr;
	}

	/** Return the locomotion params from animations array. */
	FORCEINLINE const FLocomotionParams* GetLocomotionParamsConst(const uint8& InParamsIndex) const
	{
		return LocomotionParams.IsValidIndex(InParamsIndex) ? &LocomotionParams[InParamsIndex] : nullptr;
	}

#if 1 // Virtual Animation Skills Only
	/** Return the animation skill params from animations array. */
	FORCEINLINE virtual const FAnimSkillParams* GetParamsByIndex(const uint8& InParamsIndex) const
	{
		return LocomotionParams.IsValidIndex(InParamsIndex) ? &LocomotionParams[InParamsIndex] : nullptr;
	}
#endif

#pragma endregion


#pragma region Editor

public: // IBoneReferenceSkeletonProvider
	class USkeleton* GetSkeleton(bool& bInvalidSkeletonIsError) override;

#if WITH_EDITORONLY_DATA

	/** Skeleton used by the first animation asset */
	UPROPERTY(EditAnywhere, Category = Editor)
	USkeleton* Skeleton;

	/** Physics asset used by the first animation asset */
	UPROPERTY(EditDefaultsOnly, Category = Editor)
	UPhysicsAsset* PhysicsAsset;

	/** Templates set in batches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Editor)
	FLocomotionParams TemplateParams;

	/** Templates set in batches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Editor)
	TMap<ELocomotionStatus, int32> MontageSlotDataIndexMap;

	/** Templates montage slots name in batches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Editor, meta = (CustomizeProperty))
	TArray<FSkillMontageSlotData> MontageSlotsData;

#endif

#if WITH_EDITOR
public:

	/** Reset all config variables to default. */
	UFUNCTION(CallInEditor, Category = Editor)
	void ResetConfig();

	/** Sort Locomotion params from optimization array. */
	UFUNCTION(CallInEditor, Category = Editor)
	virtual void SortParamsIndex();
	void OnSortLocomotionParamsIndex(uint8& InIndex, TArray<FLocomotionParams>& InLocomotionParams, ELocomotionStatus InStatus, int32 InGaitAxisRange = 0);
	void OnSortLocomotionParamsIndex(uint8& InIndex, FLocomotionParams& InLocomotionParams, ELocomotionStatus InStatus, int32 InGaitAxisRange = 0);

	/** Bake Locomotion params montage slots name. */
	UFUNCTION(CallInEditor, Category = Editor)
	void BakeParamsMontageSlotsName();
	void OnBakeLocomotionParamsMontageSlotsName(TArray<FLocomotionParams>& InLocomotionDirectionParams);
	void OnBakeLocomotionParamMontageSlotsName(FLocomotionParams& InLocomotionDirectionParam);
	TArray<FName> GetMontageSlotName(const ELocomotionStatus InStatus);

#endif // WITH_EDITOR
#pragma endregion
};
