// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VMM_Types.h"
#include "Abilities/VMM_GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "VMM_GameplayAbility_Locomotion.generated.h"

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_GameplayAbility_Locomotion : public UVMM_GameplayAbility
{
	GENERATED_UCLASS_BODY()
	
public:

	/** Actually activate ability, do not call this directly */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle InHandle
		, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
		, const FGameplayEventData* InTriggerEventData) override;

	/** Native function, called if an ability ends normally or abnormally. If bReplicate is set to true, try to replicate the ending to the client/server */
	virtual void EndAbility(const FGameplayAbilitySpecHandle InHandle
		, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
		, bool bReplicateEndAbility, bool bWasCancelled) override;

#pragma region Virtual Animation Skills

protected:

	/** Play Skill: Montage blend out event */
	virtual void OnAnimSkillMontageBlendOutEvent(UAnimMontage* InMontage, bool bInterrupted) override;

	/** Play Skill: Montage blend ended event */
	virtual void OnAnimSkillMontageBlendEndedEvent(UAnimMontage* InMontage, bool bInterrupted) override;

#pragma endregion


#pragma region Config

protected:

	/** Movement mode tags */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	TMap<TEnumAsByte<EMovementMode>, FGameplayTag> MovementModeTags;

	/** Jump apex state tag */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FGameplayTag JumpApexTag;

	/** Stop animation early blend out tag */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FGameplayTagContainer EarlyStopBlendOutTags;

#pragma endregion


#pragma region Task

protected:

	/** Current active locomotion status */
	ELocomotionStatus LocomotionStatus;

	/** Cached last locomotion status */
	ELocomotionStatus LastLocomotionStatus;

	/** Cached trajectory gait axis */
	TOptional<int32> TrajectoryGaitAxis;

	/** Cached trajectory locomotion status */
	TOptional<ELocomotionStatus> TrajectoryLocomotionStatus;

	/** Current active locomotion data reference */
	UPROPERTY(Transient)
	const UVMM_LocomotionData* LocomotionData;

	/** Cached last locomotion data reference */
	UPROPERTY(Transient)
	const UVMM_LocomotionData* LastLocomotionData;

public:

	/** Tick function for this task, if bTickingTask == true */
	//virtual void TickTask(float InDeltaSeconds) override;

	/** Return whether the locomotion animation can be response */
	virtual bool CanResponseMovement();

public:

	/** Set new locomotion data and cache current data asset and handle animation blending */
	UFUNCTION()
	void SetLocomotionData(const UVMM_LocomotionData* InData);

	/** Set new locomotion status */
	void SetLocomotionStatus(const ELocomotionStatus& InStatus);

	/** If the current cached status is not equal to the expected status, we should reset */
	void ResetSameLocomotionStatus(const ELocomotionStatus& InStatus);

public:

	/** Return cached locomotion montage */
	UAnimMontage* GetLocomotionMontage(const ELocomotionStatus* InStatus = nullptr) const;

	/** Return cached locomotion params */
	const FLocomotionParams* GetLocomotionParams() const;

	/** Return cached locomotion status */
	FORCEINLINE const ELocomotionStatus& GetLocomotionStatus() const { return LocomotionStatus; }

#pragma endregion


#pragma region Input

protected:

	/** Cache response last movement input time, we will check and play turn in place animation */
	double LastResponseMovementInputTime;

	/** Return desired input angle direction. Example: Start, Pivot */
	EAngleDirection GetDesiredInputDirection(FRotator* OutRotation = nullptr) const;

	/** Response movement direction changed callback from movement component - LocallyControlled */
	UFUNCTION()
	virtual void ResponseMovementDirection(UVMM_CharacterMovementComponent* InMovementComp);

	/** Response movement ability input changed callback from movement component - LocallyControlled */
	UFUNCTION()
	virtual void ResponseMovementAbility(UVMM_CharacterMovementComponent* InMovementComp, bool InTrigger);

#pragma endregion


#pragma region Movement

	/** Cached lock rotation input state */
	uint8 bLockRotationInput : 1;

	/** Response movement jump apex callback from character - Server/Client */
	UFUNCTION()
	virtual void OnJumpApexCallback();

	/** Response movement landed callback from character - Server/Client */
	UFUNCTION()
	virtual void OnLandedCallback(const FHitResult& Hit);

	/** Response face rotation changed, @See ACharacter::FaceRotation() - Client */
	UFUNCTION()
	virtual void OnFaceRotation(const float& InDeltaSeconds, const FRotator& InControlRotation);
	
	/** Response rotation mode changed callback from character - Any */
	UFUNCTION()
	virtual void OnRotationModeChanged(const ERotationMode& InRotationMode, const ERotationMode& InLastRotationMode, const ERotationMode& InLastLookingRotationModee);

	/** Response movement mode changed callback from character - Any */
	UFUNCTION()
	virtual void OnMovementModeChange(ACharacter* InCharacter, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

#pragma endregion


#pragma region Gait

	/** Re-correct valid gait axis */
	virtual bool AdjustGaitAxis(AVMM_Character* InCharacter);

	/** Response character gait state changed */
	UFUNCTION()
	virtual void OnGaitChanged(AVMM_Character* InCharacter);

#pragma endregion


#pragma region Transition

protected:

	/** Cached transition montage */
	UPROPERTY(Transient)
	UAnimMontage* TransitionMontage;

	/** Response play moving animation */
	virtual bool ResponseDataTransition();

#pragma endregion


#pragma region Turn

protected:

	/** Cached turn in place montage */
	UPROPERTY(Transient)
	UAnimMontage* TurnMontage;

	/** Cached turn in place montage rotation angle */
	float TurnAngle;

	/** Cached response turn accumulate time in tick */
	float ResponseTurnAccumulateTime;

	/** Return whether the change turn animation can be response */
	virtual bool CanChangeTurn(const UVMM_LocomotionData* InData, const FLocomotionParams* InParams
		, bool* OutResult = nullptr, float* OutBlendTime = nullptr);

	/** Update play turn in place animation */
	virtual bool UpdateTurnInPlace(const float InDeltaSeconds);

	/** Response play turn in place animation */
	virtual bool ResponseTurnInPlace(const FIntPoint& InGatAxisRange, const EAngleDirection& InDirection);

#pragma endregion


#pragma region Start

protected:

	/** Cached start montage */
	UPROPERTY(Transient)
	UAnimMontage* StartMontage;

	/** Cached start montage direction */
	UPROPERTY(Transient)
	ECardinalDirection StartMontageDirection;

	/** Response play start animation */
	virtual bool ResponseMovementStart(const FIntPoint& InGatAxisRange, const EAngleDirection& InFaceDirection, const ECardinalDirection& InDirection);

#pragma endregion


#pragma region Moving

protected:

	/** Cached moving montage */
	UPROPERTY(Transient)
	UAnimMontage* MovingMontage;

	/** Cached record moving montages, We should avoid repeating too many animations, or we should revive them */
	UPROPERTY(Transient)
	TArray<UAnimMontage*> RecordMovingMontages;

	/** Return whether the change moving animation can be response */
	virtual bool CanResponseChangeMoving(const UVMM_LocomotionData* InData, const FLocomotionParams* InParams
		, bool* OutResult = nullptr, float* OutBlendTime = nullptr);

	/** Response play moving animation */
	virtual bool ResponseMovementLoop(const FIntPoint& InGatAxisRange, const ECardinalDirection& InDirection);

#pragma endregion


#pragma region Pivot

protected:

	/** Cached pivot montage */
	UPROPERTY(Transient)
	UAnimMontage* PivotMontage;

	/** Cached response pivot time in event */
	double ResponsePivotTime;

	/** Cached response pivot accumulate time in tick */
	float ResponsePivotAccumulateTime;

	/** Update play pivot animation */
	virtual bool UpdateMovementPivot(const float InDeltaSeconds = 0.f, ECardinalDirection* InLastDirection = nullptr);

	/** Response play pivot animation */
	virtual bool ResponseMovementPivot(const FIntPoint& InGatAxisRange, const EAngleDirection& InFaceDirection, const ECardinalDirection& InDirection);

#pragma endregion


#pragma region Stop

protected:

	/** Cached stop montage */
	UPROPERTY(Transient)
	UAnimMontage* StopMontage;

	/** Cached stop montage direction */
	UPROPERTY(Transient)
	ECardinalDirection StopMontageDirection;

	FDelegateHandle EarlyStopBlendOutEventHandle;

	/** Response play stop animation */
	virtual bool ResponseMovementStop(const FIntPoint& InGatAxisRange, const ECardinalDirection& InDirection);

	/** Response early blend out event from animation */
	virtual void OnEarlyStopBlendOutEvent(FGameplayTag InEventTag, const FGameplayEventData* InPayload);

#pragma endregion


#pragma region Jump

protected:

	/** Cached falling stat location */
	UPROPERTY(Transient)
	FVector FallingStartLocation;

	/** Cached jump montage */
	UPROPERTY(Transient)
	UAnimMontage* JumpMontage;

	/** Cached falling montage */
	UPROPERTY(Transient)
	UAnimMontage* FallingMontage;

	/** Response play jump animation */
	virtual bool ResponseMovementJump(UVMM_CharacterMovementComponent* InMovementComp, const ELocomotionStatus InStatus);

	/** Response movement jump apex notify hit - Server/Client */
	UFUNCTION()
	virtual void OnJumpApex(FGameplayEventData InPayload);

#pragma endregion


#pragma region Foot Position

protected:

	/** Return foot index from last locomotion animation */
	int32 GetFootPosition();

#pragma endregion
};
