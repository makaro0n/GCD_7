// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "AlphaBlend.h"
#include "Animation/Skeleton.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include <Animation/AnimationAsset.h>
#include "VMM_Types.generated.h"

class UBlendSpace;
class UAnimMontage;
class UAnimSequenceBase;
class UVMM_LocomotionData;

/** Types of Motion Matching curve tools */
UENUM()
enum class EMotionCurveType : uint8
{
	Speed,
	SpeedAlpha,
	ForwardMovement,
	ForwardMovementAlpha,
	RightMovement,
	RightMovementAlpha,
	UpMovement,
	UpMovementAlpha,
	TranslationAlpha,
	PitchRotation,
	PitchRotationAlpha,
	YawRotation,
	YawRotationAlpha,
	RollRotation,
	RollRotationAlpha,
	RotationAlpha,
	MAX UMETA(Hidden)
};

/** An enumeration for the different methods of transitioning between animations in motion matching */
UENUM(BlueprintType)
enum class EVirtualTransitionMethod : uint8
{
	None,
	Blend,
	Inertialization
};

UENUM(BlueprintType)
enum class EInputDirection : uint8
{
	Forward,
	ForwardLeft,
	ForwardRight,
	Left,
	Right,
	Backward,
	BackwardLeft,
	BackwardRight,
	MAX UMETA(Hidden)
};

/** Types of virtual motion cardinal direction */
UENUM(BlueprintType)
enum class ECardinalDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right,
	MAX UMETA(Hidden)
};

/** Types of virtual motion angle direction */
UENUM(BlueprintType)
enum class EAngleDirection : uint8
{
	ForwardLF,
	ForwardRF,
	ForwardLeft,
	ForwardRight,
	Left,
	Right,
	BackwardLeft,
	BackwardRight,
	BackwardLF,
	BackwardRF,
	MAX UMETA(Hidden)
};

/** Types of camera view mode **/
UENUM(BlueprintType)
enum class ECameraViewMode : uint8
{
	FirstPerson,
	ThirdPerson,
	TopDownPerson,
	MAX UMETA(Hidden)
};

/** Types of rotation mode **/
UENUM(BlueprintType)
enum class ERotationMode : uint8
{
	Relaxed,
	UnEasy,
	Combat,
	Velocity,
	MAX UMETA(Hidden)
};

/** Types of virtual motion matching status */
UENUM(BlueprintType)
enum class ELocomotionStatus : uint8
{
	STOP		UMETA(DisplayName = "Stop"),
	START		UMETA(DisplayName = "Start"),
	MOVING	    UMETA(DisplayName = "Moving"),
	PIVOT		UMETA(DisplayName = "Pivot"),
	TRANSITION		UMETA(DisplayName = "Transition"),
	TURNINPLACE		UMETA(DisplayName = "Turn InPlace"),
	JUMPSTART		UMETA(DisplayName = "Jump Start"),
	JUMPFALLING	    UMETA(DisplayName = "Jump Falling"),
	JUMPPREDICTLAND		UMETA(DisplayName = "Jump Predict Land"),
	JUMPLAND		UMETA(DisplayName = "Jump Land"),
	JUMPMOBILELAND		UMETA(DisplayName = "Jump Mobile Land"),
	MAX		UMETA(Hidden),
};

/** Types of virtual motion stop section */
UENUM(BlueprintType)
enum class ELocomotionStopSection : uint8
{
	Normal,
	Immediate,
	MAX UMETA(Hidden)
};

#if 1 // Virtual Animation Skills
/** Struct of animation asset montages slot data */
USTRUCT(BlueprintType)
struct FSkillMontageSlotData
{
	GENERATED_USTRUCT_BODY()

	/** Display the montage slot data text */
	UPROPERTY(EditAnywhere, Category = "Montage Params")
	FText DisplayText;

	/** Slots name used by the current montages */
	UPROPERTY(EditAnywhere, Category = "Montage Params")
	TArray<FName> SlotsName;

	FSkillMontageSlotData()
	{
		SlotsName.AddUnique(FAnimSlotGroup::DefaultSlotName);
	}
};

/** Structure for animation data in the VirtualAnimationSkill. */
USTRUCT(BlueprintType)
struct FAnimSkillParams
{
	GENERATED_USTRUCT_BODY()
	
	/** Represents the global index for this parameter, through which we will do quick queries */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Settings)
	int32 Index;

	/** Flag the animation is looping asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	uint8 bLooping : 1;

	/** The animation sequence asset to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	UAnimSequenceBase* Asset;

	/** The animation sequence asset to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	UAnimMontage* Montage;

	/** Sync group settings for this player.  Sync groups keep related animations with different lengths synchronized. */
	UPROPERTY(EditDefaultsOnly, Category = Settings)
	FAnimationGroupReference SyncGroup;

	/** Animation initialize play rate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float PlayRate;

	/** Animation initialize play start position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float StartPosition;

	/** Animation meta play start position, It can be used as any dynamic jump to data, such as a short emergency stop jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<float> MetaStartPosition;

	/** Animation montage blend out trigger time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float BlendOutTriggerTime;

	/** Animation asset blend in data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FAlphaBlend BlendIn;

	/** Animation asset blend out data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FAlphaBlend BlendOut;

	/** Animation montage slots name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<FName> MontageSlotsName;

	/** Animation rotation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	FRotator AnimatedRotation;

public:

	/** Return the animation params is valid */
	FORCEINLINE bool IsValid() const { return IsValidIndex() && IsValidAsset(); }

	/** Return the params index is valid */
	FORCEINLINE bool IsValidIndex() const { return Index != INDEX_NONE; }

	/** Return the animation asset is valid */
	FORCEINLINE bool IsValidAsset() const { return Asset != NULL || Montage != NULL; }

	/** Return the animation asset has rotation value */
	FORCEINLINE bool HasRotation() const { return AnimatedRotation != FRotator::ZeroRotator; }

	FAnimSkillParams()
		: Index(INDEX_NONE)
		, bLooping(false)
		, Asset(nullptr)
		, Montage(nullptr)
		, PlayRate(1.0f)
		, StartPosition(0.f)
		, BlendOutTriggerTime(-1.f)
		, BlendIn(FAlphaBlend(0.25f))
		, BlendOut(FAlphaBlend(0.25f))
		, AnimatedRotation(ForceInitToZero)
	{}
};

/** Struct of animation skill params group data */
USTRUCT()
struct FVirtualSkillParamsGroup
{
	GENERATED_USTRUCT_BODY()

	/** Animation skill data index */
	UPROPERTY()
	int32 DataIndex;

	/** Animation skill params index */
	UPROPERTY()
	int32 ParamsIndex;

public:

	/** Return the skill params in the group */
	const FAnimSkillParams* GetSkillParams() const;

	FVirtualSkillParamsGroup()
		: DataIndex(INDEX_NONE)
		, ParamsIndex(INDEX_NONE)
	{}

	FVirtualSkillParamsGroup(const int32 InDataIndex, const int32 InParamsIndex)
		: DataIndex(InDataIndex)
		, ParamsIndex(InParamsIndex)
	{}
};

#endif // Virtual Animation Skills

/** Struct of locomotion animation data */
USTRUCT(BlueprintType)
struct FLocomotionParams : public FAnimSkillParams
{
	GENERATED_USTRUCT_BODY()

	// ------------------------------------------ Static Data ------------------------------------------ //

	/** Represents the global index for this parameter, through which we will do quick queries */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Settings)
	uint8 GaitAxis;

	/** Locomotion status */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	ELocomotionStatus LocomotionStatus;

	/** Locomotion cardinal direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	ECardinalDirection CardinalDirection;

	FLocomotionParams()
		: GaitAxis(0)
		, LocomotionStatus(ELocomotionStatus::MAX)
		, CardinalDirection(ECardinalDirection::MAX)
	{
		MontageSlotsName.AddUnique(FAnimSlotGroup::DefaultSlotName);
	}
};

/** Struct of locomotion transition animations data */
USTRUCT(BlueprintType)
struct FLocomotionTransitionAnims
{
	GENERATED_USTRUCT_BODY()

	/** If true, then it only allows full body animations to be played, no need to use it as an upper body layer during movement etc. */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	uint8 bPlayInUpperLayer : 1;

	/** Transition animation params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	float BlendOutTriggerTimeInUpperLayer;

	/** Transition animation params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FAlphaBlend BlendOutInUpperLayer;

	/** Transition animation params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<FLocomotionParams> MultiParams;

	FLocomotionTransitionAnims()
		: bPlayInUpperLayer(true)
		, BlendOutTriggerTimeInUpperLayer(-1.f)
	{
		BlendOutInUpperLayer.SetBlendTime(0.4f);
		BlendOutInUpperLayer.SetBlendOption(EAlphaBlendOption::HermiteCubic);
	}
};

/** Struct of locomotion jump animations data */
USTRUCT(BlueprintType)
struct FLocomotionJumpAnims
{
	GENERATED_USTRUCT_BODY()

	/** Jump start animation params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FLocomotionParams JumpStart;

	/** Jump falling animation params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<FLocomotionParams> JumpFallings;

	/** Jump land animation params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FLocomotionParams JumpLand;

	/** Jump mobile land animation params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FLocomotionParams JumpMobileLand;

	FLocomotionJumpAnims()
	{}
};

/** Struct of locomotion gait animations data */
USTRUCT(BlueprintType)
struct FLocomotionGaitAnims
{
	GENERATED_USTRUCT_BODY()

	/** Description for the current gait animations */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FName Tag;

	/** Movement circle incremental angle */
	UPROPERTY(EditDefaultsOnly, Category = Data, meta = (UIMin = "0.0", ClampMin = "0.0"))
	TArray<float> CircleIncrementalAngles;

	/** Movement circle incremental angle */
	UPROPERTY(EditDefaultsOnly, Category = Data, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
	TArray<float> RotationOffsetAlpha;

	/** Movement rotation offset animations blend space asset as cardinal direction */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<UBlendSpace*> RotationOffsetAnims;

	/** Idle animation params index range in global params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FIntPoint IdleAnimsIndicesRange;

	/** Turn animation type will be randomly selected based on this range, in units of 10 */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FIntPoint TurnAnimsIndicesRange;

	/** Movement start animations, 0 - 9 is forward start animation, 10 - 12 is other cardinal direction */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<FLocomotionParams> StartAnims;

	/** Movement loop animations, cardinal direction * (moving + left arch + right arch) */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<FLocomotionParams> MovingAnims;

	/** Movement pivot animations, 0 - 9 is forward pivot animation, 10 - 12 is other cardinal direction */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<FLocomotionParams> PivotAnims;

	/** Movement stop animations, cardinal direction * (left foot + right foot) */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<FLocomotionParams> StopAnims;

	/** Movement jump animations */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<FLocomotionJumpAnims> JumpAnims;

	FLocomotionGaitAnims()
		: Tag(NAME_None)
		, IdleAnimsIndicesRange(INDEX_NONE)
		, TurnAnimsIndicesRange(INDEX_NONE)
	{
		CircleIncrementalAngles.Init(3.f, 5);
		RotationOffsetAnims.AddDefaulted();
	}
};

/** Struct of locomotion gait attribute animations data */
USTRUCT(BlueprintType)
struct FLocomotionGaitAttributeData
{
	GENERATED_USTRUCT_BODY()

	/** Description for the current gait animations */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FName Tag;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	uint8 bResponseMovementStartInUp : 1;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	uint8 bResponseMovementStartInDown : 1;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	uint8 bResponseMovementStopInUp : 1;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	uint8 bResponseMovementStopInDown : 1;

	/** Current gait axis to other gait axis skill params */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FLocomotionParams GaitChangedParams;

	FLocomotionGaitAttributeData()
		: Tag(NAME_None)
		, bResponseMovementStartInUp(false)
		, bResponseMovementStartInDown(false)
		, bResponseMovementStopInUp(false)
		, bResponseMovementStopInDown(false)
	{}
};

/** Struct of locomotion gait attributes animations data */
USTRUCT(BlueprintType)
struct FLocomotionGaitAttributesData
{
	GENERATED_USTRUCT_BODY()

	/** Description for the current gait animations */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FName Tag;

	/** Response pivot in gait axis */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	uint8 bResponsePivot : 1;

	/** Why? */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	uint8 bIgnorePoseMatchingByPivot : 1;

	/** Response transition stop in gait axis */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	uint8 bResponseTransitionStop : 1;

	/** The animation type will be randomly selected based on this range */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FIntPoint RandomIndexRange;

	/** We will randomize the playback magnification of the animation during initialization */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FVector2D RandomRateRange;

	/** The turn animation type will be randomly selected based on this range, in units of 10 */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FIntPoint RandomTurnInPlaceIndexRange;

	/** Gait axis to other gait axis attribute data */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<FLocomotionGaitAttributeData> AttributesData;

	/** Whether to ignore the specified movement direction, it will be an invalid asset */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	TArray<ECardinalDirection> IgnoreCarinalDirections;

	FLocomotionGaitAttributesData()
		: Tag(NAME_None)
		, bResponsePivot(true)
		, bIgnorePoseMatchingByPivot(false)
		, bResponseTransitionStop(true)
		, RandomIndexRange(0)
		, RandomRateRange(FVector2D::ZeroVector)
		, RandomTurnInPlaceIndexRange(INDEX_NONE)
	{}
};

/** Struct of locomotion animations style data */
USTRUCT(BlueprintType)
struct FMotionAnimDataset : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

#if WITH_EDITORONLY_DATA
	/** Description of the current animation type. */
	UPROPERTY(EditDefaultsOnly, Category = Editor)
	FText Description;
#endif // WITH_EDITORONLY_DATA

	/** Animation state locomotion data asset, Stand,Crouch,Prone - Relaxed etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	TMap<FGameplayTag, class UVMM_LocomotionData*> RlxLocomotionDataset;

	/** Animation state locomotion data asset, Stand,Crouch,Prone - Aim etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	TMap<FGameplayTag, class UVMM_LocomotionData*> AimLocomotionDataset;

	FMotionAnimDataset()
	{}
};
