// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Interfaces/Interface_BoneReferenceSkeletonProvider.h"
#include "VMM_Types.h"
#include "DataRegistryId.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "VMM_AnimInstance.generated.h"

class AVMM_Character;
class UVMM_LocomotionData;
class UVMM_CharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocomotionDataSignature, const UVMM_LocomotionData*, Data);

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_AnimInstance : public UAnimInstance, public IBoneReferenceSkeletonProvider
{
	GENERATED_UCLASS_BODY()

public:

#pragma region Editor

#if WITH_EDITORONLY_DATA
	/** Using editor style */
	UPROPERTY(EditDefaultsOnly, Category = Editor)
	USkeleton* EditorSkeleton;
#endif // WITH_EDITORONLY_DATA

	/** Return desired animation skeleton */
	class USkeleton* GetSkeleton(bool& bInvalidSkeletonIsError) override { bInvalidSkeletonIsError = false; return CurrentSkeleton; }

#pragma endregion


#pragma region Owner

protected:

	/** Anim Instance belongs to */
	UPROPERTY(Transient, DuplicateTransient)
	AVMM_Character* CharacterOwner;

	/** Custom character movement component reference */
	UPROPERTY(Transient, DuplicateTransient)
	UVMM_CharacterMovementComponent* CharacterMovementComp;

	// the below functions are the native overrides for each phase
	// Native initialization override point
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float InDeltaSeconds) override;
	virtual void Montage_Advance(float InDeltaSeconds) override;

#pragma endregion


#pragma region Eye

protected:

	/** First person camera bone reference */
	UPROPERTY(EditDefaultsOnly, Category = "Eye")
	FSocketReference EyeSocketReference;

public:

	/** Return eye view location */
	virtual FVector GetEyeLocation() const;

	/** Return eye socket name */
	FORCEINLINE const FName& GetEyeSocketName() const { return EyeSocketReference.SocketName; }

#pragma endregion


#pragma region Data

protected:

	/** Initialize animation style registry id */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	FDataRegistryId InitAnimRegistryId;

	/** Active animation style registry id */
	UPROPERTY(Transient)
	FDataRegistryId AnimRegistryId;

	/** Active animation dataset */
	UPROPERTY(Transient)
	FMotionAnimDataset AnimDataset;

	/** Current active locomotion data reference */
	UPROPERTY(Transient)
	const UVMM_LocomotionData* FutureLocomotionData;

	/** Current active locomotion data reference */
	UPROPERTY(Transient)
	const UVMM_LocomotionData* PreviousLocomotionData;

public:

	/** Initialize default animation registry id */
	virtual bool InitializeAnimRegistryId();

	/** Set new data registry id and cache current and find desired animations */
	virtual bool SetAnimRegistryId(const FDataRegistryId& InRegistryId);

	/** Update locomotion data from cached animation dataset */
	virtual bool UpdateLocomotionData();

	/** Set new locomotion data and cache current data asset and handle animation blending */
	virtual void SetLocomotionData(const UVMM_LocomotionData* InData);

	/*-----------------------------------------------------------------------------
		Blueprint Callable.
	-----------------------------------------------------------------------------*/

	/** Set new data registry id and cache current and find desired animations */
	UFUNCTION(BlueprintCallable, Category = "Data", meta = (DisplayName = "Set Anim Registry Id"))
		bool K2_SetAnimRegistryId(FDataRegistryId InRegistryId) { return SetAnimRegistryId(InRegistryId); }


public:

	/** Locomotion data delegates */
	FLocomotionDataSignature LocomotionDataDelegate;

	/** Return cached locomotion data reference */
	FORCEINLINE const UVMM_LocomotionData* GetLocomotionData() const { return bIsPreviousLayer ? PreviousLocomotionData : FutureLocomotionData; }

	/** Return cached last locomotion data reference */
	FORCEINLINE const UVMM_LocomotionData* GetLastLocomotionData() const { return !bIsPreviousLayer ? PreviousLocomotionData : FutureLocomotionData; }

public:

	/** Returns the character locomotion gait axis range */
	const FIntPoint GetGaitAxisRange() const;

	/** Return future base pose animation asset */
	UFUNCTION(BlueprintPure, Category = Animation)
	UAnimSequence* GetFutureBasePose() const;

	/** Return future aim offset animation asset */
	UFUNCTION(BlueprintPure, Category = Animation)
	UBlendSpace* GetFutureAimOffset() const;

	/** Return future rotation offset animation asset */
	UFUNCTION(BlueprintPure, Category = Animation)
	UBlendSpace* GetFutureRotationOffset() const;

	/** Return previous base pose animation asset */
	UFUNCTION(BlueprintPure, Category = Animation)
	UAnimSequence* GetPreviousBasePose() const;

	/** Return future aim offset animation asset */
	UFUNCTION(BlueprintPure, Category = Animation)
	UBlendSpace* GetPreviousAimOffset() const;

	/** Return future rotation offset animation asset */
	UFUNCTION(BlueprintPure, Category = Animation)
	UBlendSpace* GetPreviousRotationOffset() const;

#pragma endregion


#pragma region State

protected:

	/** Stand gameplay tag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State")
	FGameplayTag StandTag;

	/** Crouch gameplay tag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State")
	FGameplayTag CrouchTag;

	/** Response character stance state changed */
	UFUNCTION()
	virtual void OnStanceChanged(ACharacter* InCharacter);

#pragma endregion


#pragma region Pose Layer

protected:

	/** Flag is first person - Locally */
	bool bIsFirstPerson;

public:

	/** Set first person mode */
	virtual void SetFirstPersonMode(bool InState);

	/** Set first person mode - Blueprint */
	UFUNCTION(BlueprintImplementableEvent, Category = "First Person", meta = (DisplayName = "Set First Person Mode"))
	void K2_SetFirstPersonMode(bool InState);

#pragma endregion


#pragma region Animation Layer

protected:

	/** Flag animation layer is previous or future */
	uint8 bIsPreviousLayer : 1;

	/** Flag animation layer weight */
	float AnimLayerWeight;

	/** Flag animation layer weight */
	float LastAnimLayerWeight;

public:

	/** Revert to the last weight value */
	virtual void RevertBeforeLayerWeight();

	/** Revert to the current weight value */
	virtual void RevertAfterLayerWeight();

public:

	/** Return the previous body layer condition */
	UFUNCTION(BlueprintPure, Category = AnimLayer, meta = (NotBlueprintThreadSafe))
	FORCEINLINE bool IsPreviousLayerWeight() const { return bIsPreviousLayer; }

	/** Return the previous body layer weight */
	UFUNCTION(BlueprintPure, Category = AnimLayer, meta = (NotBlueprintThreadSafe))
	FORCEINLINE float GetPreviousLayerWeight() const { return AnimLayerWeight; }

#pragma endregion


#pragma region Lower Body Layer

protected:

	/** Cached lower body montage slot weight */
	UPROPERTY(Transient)
	float LowerBodyLayerWeight;

	/** Cached lower body layer inverse upper layer weight */
	UPROPERTY(Transient)
	float LowerBodyLayerInverseUpperLayerWeight;

	/** Cached lower body montage */
	UPROPERTY(Transient)
	UAnimMontage* LowerBodyMontage;

	/** Cached lower body foot lock transition montage */
	UPROPERTY(Transient)
	UAnimMontage* LowerBodyLockMontage;

public:

	/** Lower body montage slot data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lower Body", meta = (CustomizeProperty))
	FSkillMontageSlotData LowerBodySlotData;

public:

	/** Set lower body montage reference */
	void SetLowerBodyMontage(UAnimMontage* InMontage);

	/** Set lower body foot lock transition montage reference */
	void SetLowerBodyLockMontage(UAnimMontage* InMontage);

	/** Return the montage is lower body layer condition */
	bool IsLowerBodyLayerMontage(UAnimMontage* InMontage) const;
	bool IsLowerBodyLayerMontageSlot(const TArray<FName>& InSlotsName) const;

	/** Return lower body montage play state from  AnimInstance */
	virtual bool IsPlayingLowerBodyMontage(const bool InActiveMontage = true);

public:

	/** Return the lower body montage reference */
	FORCEINLINE UAnimMontage* GetLowerBodyMontage() const { return LowerBodyMontage; }

	/** Return the lower body lock montage reference */
	FORCEINLINE UAnimMontage* GetLowerBodyLockMontage() const { return LowerBodyLockMontage; }

	/** Return the lower body layer weight */
	UFUNCTION(BlueprintPure, Category = AnimLayer, meta = (NotBlueprintThreadSafe))
	FORCEINLINE float GetLowerBodyLayerWeight() const { return LowerBodyLayerWeight; }

	/** Return the lower body layer inverse upper body weight */
	UFUNCTION(BlueprintPure, Category = AnimLayer, meta = (NotBlueprintThreadSafe))
	FORCEINLINE float GetLowerBodyLayerInverseUpperLayerWeight() const { return LowerBodyLayerInverseUpperLayerWeight; }

#pragma endregion


#pragma region Upper Body Layer

protected:

	/** Cached lower body montage slot weight */
	UPROPERTY(Transient)
	float UpperBodyLayerWeight;

	/** Cached upper body montage */
	UPROPERTY(Transient)
	UAnimMontage* UpperBodyMontage;

	/** Cached ignore other body montage as upper */
	UPROPERTY(Transient)
	UAnimMontage* UpperIgnoreBodyMontage;

public:

	/** Upper body montage slot data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upper Body", meta = (CustomizeProperty))
	FSkillMontageSlotData UpperBodySlotData;

public:

	/** Set upper body montage reference */
	void SetUpperBodyMontage(UAnimMontage* InMontage);

	/** Set ignore calculate other body montage reference */
	void SetUpperIgnoreBodyMontage(UAnimMontage* InMontage);

	/** Return the montage is upper body layer condition */
	bool IsUpperBodyLayerMontage(UAnimMontage* InMontage) const;
	bool IsUpperBodyLayerMontageSlot(const TArray<FName>& InSlotsName) const;

	/** Return upper body montage play state from  AnimInstance */
	virtual bool IsPlayingUpperBodyMontage(const bool InActiveMontage = true);

public:

	/** Return the upper body layer weight */
	UFUNCTION(BlueprintPure, Category = AnimLayer, meta = (NotBlueprintThreadSafe))
	FORCEINLINE float GetUpperBodyLayerWeight() const { return UpperBodyLayerWeight; }

	/** Return the upper body montage reference */
	FORCEINLINE UAnimMontage* GetUpperBodyMontage() const { return UpperBodyMontage; }

#pragma endregion


#pragma region Whole Body Layer

protected:

	/** Cached lower body montage slot weight */
	UPROPERTY(Transient)
	float WholeBodyLayerWeight;

	/** Cached whole body montage */
	UPROPERTY(Transient)
	UAnimMontage* WholeBodyMontage;

public:

	/** Whole body montage slot data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whole Body", meta = (CustomizeProperty))
	FSkillMontageSlotData WholeBodySlotData;

public:


	/** Set whole body montage reference */
	void SetWholeBodyMontage(UAnimMontage* InMontage);

	/** Return the montage is whole body layer condition */
	bool IsWholeBodyLayerMontage(UAnimMontage* InMontage) const;
	bool IsWholeBodyLayerMontageSlot(const TArray<FName>& InSlotsName) const;

	/** Return whole body montage play state from  AnimInstance */
	virtual bool IsPlayingWholeBodyMontage(const bool InActiveMontage = true);

public:

	/** Return the whole body layer weight */
	UFUNCTION(BlueprintPure, Category = AnimLayer, meta = (NotBlueprintThreadSafe))
	FORCEINLINE float GetWholeBodyLayerWeight() const { return WholeBodyLayerWeight; }

	/** Return the whole body montage reference */
	FORCEINLINE UAnimMontage* GetWholeBodyMontage() const { return WholeBodyMontage; }

#pragma endregion


#pragma region View

protected:

	/** Flag whether to enable aim offset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim Offset")
	uint8 bEnableAimOffset : 1;

	/** Current aim offset weight */
	UPROPERTY(BlueprintReadOnly, Category = "Aim Offset", meta = (EditCondition = "bEnableAimOffset"))
	float AimOffsetWeight;

	/** Cached desired aim offset weight */
	UPROPERTY(BlueprintReadOnly, Category = "Aim Offset", meta = (EditCondition = "bEnableAimOffset"))
	float DesiredAimOffsetWeight;

	/** Aim offset blend input 2d, Yaw - Pitch */
	UPROPERTY(BlueprintReadOnly, Category = "Aim Offset", meta = (EditCondition = "bEnableAimOffset"))
	FVector2D AimOffsetBlendInput;

	/** Aim offset interp speed */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Offset", meta = (EditCondition = "bEnableAimOffset"))
	float AimOffsetInterpSpeed;

	/** Aim offset weight interp speed */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Offset", meta = (EditCondition = "bEnableAimOffset"))
	float AimOffsetWeightInterpSpeed;

	/** Calculate view rotation */
	virtual void CalculateViewRotation(const float& InDeltaSeconds);

public:

	/** Return current aim offset blend input. */
	FORCEINLINE const FVector2D& GetAimOffsetBlendInput() { return AimOffsetBlendInput; }

#pragma endregion


#pragma region Rotation

protected:

	/** Cached character rotation from last updated */
	UPROPERTY(Transient)
	FRotator LastCharacterRotation;

	/** Max additive rotation yaw angle range */
	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	FVector2D RotationYawOffsetRange;

	/** Rotation offset of the current frame from the previous frame */
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	FVector2D RotationOffset;

	/** Rotation offset alpha of the current frame from the previous frame */
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	float RotationOffsetAlpha;

	/** Return rotation offset additive alpha */
	virtual float GetRotationOffsetAlpha();

	/** Calculate rotation offset in the frame */
	virtual void CalculateRotationOffset(const float& InDeltaSeconds);

#pragma endregion
};
