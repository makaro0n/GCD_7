// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Abilities/VMM_GameplayAbility_Locomotion.h"
#include "GameFramework/VMM_CharacterMovementComponent.h"
#include "VirtualMotionMatching.h"
#include "Librarys/VMM_Library.h"
#include "DataAssets/VMM_LocomotionData.h"
#include "Abilities/VMM_AbilitySystemComponent.h"
#include "Librarys/VMM_MontageLibrary.h"
#include "Librarys/VMM_PoseSearchLibrary.h"
#include "Librarys/VMM_CurveLibrary.h"
#include "GameFramework/VMM_AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameFramework/VMM_Character.h"
#include "VMM_Settings.h"

UVMM_GameplayAbility_Locomotion::UVMM_GameplayAbility_Locomotion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LocomotionStatus(ELocomotionStatus::MAX)
	, LastLocomotionStatus(ELocomotionStatus::MAX)
	, LocomotionData(nullptr)
	, LastLocomotionData(nullptr)
	, StartMontage(nullptr)
	, MovingMontage(nullptr)
	, PivotMontage(nullptr)
	, ResponsePivotAccumulateTime(0.f)
	, StopMontage(nullptr)
	, JumpMontage(nullptr)
	, FallingMontage(nullptr)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Initialize movement mode tags
	for (int32 Index = 0; Index < EMovementMode::MOVE_MAX ; Index++)
	{
		MovementModeTags.FindOrAdd(EMovementMode(Index));
	}
}

void UVMM_GameplayAbility_Locomotion::ActivateAbility(const FGameplayAbilitySpecHandle InHandle
	, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
	, const FGameplayEventData* InTriggerEventData)
{
	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = Cast<UVMM_CharacterMovementComponent>(InActorInfo->MovementComponent.Get());
	if (theMovementComp == nullptr)
	{
		EndAbility(InHandle, InActorInfo, InActivationInfo, true, false);
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::ActivateAbility, Invalid virtual motion matching movement component."));
		return;
	}

	// Bind the movement direction changed delegate
	theMovementComp->MovementDirectionDelegate.AddUniqueDynamic(this, &ThisClass::ResponseMovementDirection);
	theMovementComp->MovementAbilityDelegate.AddUniqueDynamic(this, &ThisClass::ResponseMovementAbility);

	// Bind character movement mode changed delegate
	if (AVMM_Character* theCharacter = Cast<AVMM_Character>(theMovementComp->GetCharacterOwner()))
	{
		theCharacter->CharacterGaitDelegate.AddUniqueDynamic(this, &ThisClass::OnGaitChanged);
		theCharacter->LandedDelegate.AddUniqueDynamic(this, &ThisClass::OnLandedCallback);
		theCharacter->OnReachedJumpApex.AddUniqueDynamic(this, &ThisClass::OnJumpApexCallback);
		theCharacter->FaceRotationDelegate.AddUniqueDynamic(this, &ThisClass::OnFaceRotation);
		theCharacter->RotationModeDelegate.AddUniqueDynamic(this, &ThisClass::OnRotationModeChanged);
		theCharacter->MovementModeChangedDelegate.AddUniqueDynamic(this, &ThisClass::OnMovementModeChange);
	}

	// Check the animation instance is valid
	UVMM_AnimInstance* theAnimInstance = Cast<UVMM_AnimInstance>(InActorInfo->GetAnimInstance());
	if (theAnimInstance == nullptr)
	{
		return;
	}

	// Bind the locomotion data changed delegate
	theAnimInstance->LocomotionDataDelegate.AddUniqueDynamic(this, &ThisClass::SetLocomotionData);

	// Initialize animation dataset
	if (theAnimInstance->InitializeAnimRegistryId() == false)
	{
		SetLocomotionData(theAnimInstance->GetLocomotionData());
	}
}

void UVMM_GameplayAbility_Locomotion::EndAbility(const FGameplayAbilitySpecHandle InHandle
	, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
	, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = Cast<UVMM_CharacterMovementComponent>(InActorInfo->MovementComponent.Get());
	if (theMovementComp == nullptr)
	{
		Super::EndAbility(InHandle, InActorInfo, InActivationInfo, bReplicateEndAbility, bWasCancelled);
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::EndAbility, Invalid VMM movement component."));
		return;
	}

	// Unbind the delegates
	theMovementComp->MovementDirectionDelegate.RemoveDynamic(this, &ThisClass::ResponseMovementDirection);
	theMovementComp->MovementAbilityDelegate.RemoveDynamic(this, &ThisClass::ResponseMovementAbility);
	
	// Unbind character movement mode changed delegate
	if (AVMM_Character* theCharacter = Cast<AVMM_Character>(theMovementComp->GetCharacterOwner()))
	{
		theCharacter->LandedDelegate.RemoveDynamic(this, &ThisClass::OnLandedCallback);
		theCharacter->OnReachedJumpApex.RemoveDynamic(this, &ThisClass::OnJumpApexCallback);
		theCharacter->FaceRotationDelegate.RemoveDynamic(this, &ThisClass::OnFaceRotation);
		theCharacter->MovementModeChangedDelegate.RemoveDynamic(this, &ThisClass::OnMovementModeChange);
	}

	// Check the animation instance is valid
	UVMM_AnimInstance* theAnimInstance = Cast<UVMM_AnimInstance>(InActorInfo->GetAnimInstance());
	if (theAnimInstance == nullptr)
	{
		Super::EndAbility(InHandle, InActorInfo, InActivationInfo, bReplicateEndAbility, bWasCancelled);
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::EndAbility, Invalid VMM movement component."));
		return;
	}

	// Bind the locomotion data changed delegate
	theAnimInstance->LocomotionDataDelegate.RemoveDynamic(this, &ThisClass::SetLocomotionData);

	// Callback
	Super::EndAbility(InHandle, InActorInfo, InActivationInfo, bReplicateEndAbility, bWasCancelled);
}

#pragma region Virtual Animation Skills
void UVMM_GameplayAbility_Locomotion::OnAnimSkillMontageBlendOutEvent(UAnimMontage* InMontage, bool bInterrupted)
{
	Super::OnAnimSkillMontageBlendOutEvent(InMontage, bInterrupted);

	UE_LOG(LogVirtualMotionMatching, Verbose, TEXT("UVMM_GameplayAbility_Locomotion::OnAnimSkillMontageBlendOutEvent: %d, Montage: %s")
		, GFrameCounter, *GetNameSafe(InMontage));

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::OnAnimSkillMontageBlendOutEvent, Invalid movement component"));
		return;
	}

	// Always unlock cached rotation input
	if (bLockRotationInput)
	{
		bLockRotationInput = false;
		theMovementComp->SetIgnoreRotationInput(false);
	}

	// Get last locomotion status
	const ELocomotionStatus theLocomotionStatus = LocomotionStatus;

	// Preview handle cached locomotion status
	switch (theLocomotionStatus)
	{
	case ELocomotionStatus::TRANSITION:
		// Revert the after animation layer weight
		if (UVMM_AnimInstance* theCustomAnimInstance = Cast<UVMM_AnimInstance>(GetAnimInstanceFromActorInfo()))
		{
			theCustomAnimInstance->RevertAfterLayerWeight();
		}
		break;

	case ELocomotionStatus::JUMPLAND:
	case ELocomotionStatus::JUMPMOBILELAND:
		// Try unlock the movement input
		if (ACharacter* theChracter = theMovementComp->GetCharacterOwner())
		{
			if (AController* theOwnerController = theChracter->GetController())
			{
				theOwnerController->SetIgnoreMoveInput(false);
			}
		}
		break;
	}

	// Check the response condition
	if (CanResponseMovement() == false || !theMovementComp->IsMovingOnGround())
	{
		// Always reset the same status
		ResetSameLocomotionStatus(LocomotionStatus);
		return;
	}

	// Get current movement data
	const FIntPoint theGaitAxisRange = theMovementComp->GetGaitAxisRange();
	const EAngleDirection theMoveFootDirection = GetDesiredInputDirection();
	const ECardinalDirection theMoveDirection = theMovementComp->GetAdjustMovementCardinalDirection();

	// Response cached locomotion status
	switch (theLocomotionStatus)
	{
	case ELocomotionStatus::MOVING:
		// Since the engine cannot be modified, we can only try to modify the hybrid mode in a stand-alone situation
		if (UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo())
		{
			if (AActor* thePawnOwner = theAnimInstance->TryGetPawnOwner())
			{
				if (thePawnOwner->IsNetMode(NM_Standalone) || true)
				{
					theAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
				}
			}
		}
		break;

	case ELocomotionStatus::START:
	case ELocomotionStatus::PIVOT:
		if (theMovementComp->HasMovementInput())
		{
			// Check the trajectory status is valid
			if (TrajectoryLocomotionStatus.IsSet())
			{
				if (TrajectoryLocomotionStatus.GetValue() != theLocomotionStatus)
				{
					return;
				}
				else
				{
					TrajectoryLocomotionStatus.Reset();
				}
			}
			
			// Update response movement pivot animation
			if (UpdateMovementPivot())
			{
				return;
			}

			// Always response movement loop
			ResponseMovementLoop(theGaitAxisRange, theMoveDirection);
		}
		break;

	case ELocomotionStatus::STOP:
	case ELocomotionStatus::TRANSITION:
	case ELocomotionStatus::TURNINPLACE:
		// Check the trajectory status is valid
		if (TrajectoryLocomotionStatus.IsSet())
		{
			if (TrajectoryLocomotionStatus.GetValue() != theLocomotionStatus)
			{
				return;
			}
			else
			{
				TrajectoryLocomotionStatus.Reset();
			}
		}

		if (theMovementComp->HasMovementInput())
		{
			ResponseMovementDirection(theMovementComp);
		}

		// Always reset the same status
		ResetSameLocomotionStatus(theLocomotionStatus);
		break;

	case ELocomotionStatus::JUMPLAND:
	case ELocomotionStatus::JUMPMOBILELAND:
		// Always reset the same status
		ResetSameLocomotionStatus(theLocomotionStatus);

		// Check the trajectory status is valid
		if (TrajectoryLocomotionStatus.IsSet())
		{
			if (TrajectoryLocomotionStatus.GetValue() != theLocomotionStatus)
			{
				return;
			}
			else
			{
				TrajectoryLocomotionStatus.Reset();
			}
		}

		// Response cached movement input
		if (theMovementComp->HasMovementInput())
		{
			ResponseMovementDirection(theMovementComp);
		}
		break;
	}
}

void UVMM_GameplayAbility_Locomotion::OnAnimSkillMontageBlendEndedEvent(UAnimMontage* InMontage, bool bInterrupted)
{
	Super::OnAnimSkillMontageBlendEndedEvent(InMontage, bInterrupted);
}
#pragma endregion


#pragma region Task
bool UVMM_GameplayAbility_Locomotion::CanResponseMovement()
{
	// Check the ability system component is valid
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::CanResponseMovement, Invalid ability component"));
		return false;
	}

	// Check the block tag condition
	const bool bHasBlockTag = theAbilityComponent->HasAnyMatchingGameplayTags(ActivationBlockedTags);
	if (bHasBlockTag)
	{
		return false;
	}

	// Default
	return true;
}

void UVMM_GameplayAbility_Locomotion::SetLocomotionData(const UVMM_LocomotionData* InData)
{
	// Local function
	auto OnFailed = [&]() 
	{
		// Revert the before animation layer weight
// 		if (UVMM_AnimInstance* theCustomAnimInstance = Cast<UVMM_AnimInstance>(GetAnimInstanceFromActorInfo()))
// 		{
// 			theCustomAnimInstance->RevertBeforeLayerWeight();
// 		}
	};

	// Check the new data asset is valid
	if (InData == nullptr)
	{
		OnFailed();
		return;
	}

	// Cache current locomotion data
	LastLocomotionData = LocomotionData;

	// Response new locomotion data changed
	LocomotionData = InData;

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		OnFailed();
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::SetLocomotionData, Invalid movement component"));
		return;
	}

	// Get current movement data
	FIntPoint theGaitAxisRange = theMovementComp->GetGaitAxisRange();
	const EAngleDirection theMoveFootDirection = GetDesiredInputDirection();
	const ECardinalDirection theMoveDirection = theMovementComp->GetAdjustMovementCardinalDirection();

	// Set rotation rate
	theMovementComp->RotationRate.Yaw = LocomotionData->GetCircleIncrementalAngle(theGaitAxisRange, theMovementComp->GetRotationMode(), theMoveDirection);

	// Check the character owner is valid
	AVMM_Character* theCharacter = Cast<AVMM_Character>(theMovementComp->GetCharacterOwner());
	if (theCharacter == nullptr || !theCharacter->IsLocallyControlled())
	{
		OnFailed();
		return;
	}

	// Adjust the character gait axis
	if (AdjustGaitAxis(theCharacter)
		&& (LocomotionStatus == ELocomotionStatus::START || LocomotionStatus == ELocomotionStatus::MOVING))
	{
		OnFailed();
		return;
	}

	// Response moving on ground now
	if (!theMovementComp->IsMovingOnGround())
	{
		return;
	}

	// Check the movement input is valid
	if (theMovementComp->HasMovementInput())
	{
		// Refresh current movement data
		theGaitAxisRange = theMovementComp->GetGaitAxisRange();

		// Response movement state again
		switch (LocomotionStatus)
		{
		case ELocomotionStatus::START:
#if 1
			ResponseMovementLoop(theGaitAxisRange, theMoveDirection);
#else
			ResponseMovementStart(theGaitAxisRange, theMoveFootDirection, theMoveDirection);
#endif
			return;

		case ELocomotionStatus::MOVING:
			ResponseMovementLoop(theGaitAxisRange, theMoveDirection);
			return;
		}
	}

	// Response data transition animation
	ResponseDataTransition();
}

void UVMM_GameplayAbility_Locomotion::SetLocomotionStatus(const ELocomotionStatus& InStatus)
{
	// Check the new status is valid
	if (LocomotionStatus == InStatus)
	{
		return;
	}

	LastLocomotionStatus = LocomotionStatus;
	LocomotionStatus = InStatus;
}

void UVMM_GameplayAbility_Locomotion::ResetSameLocomotionStatus(const ELocomotionStatus& InStatus)
{
	// Check the new status is valid
	if (LocomotionStatus != InStatus)
	{
		return;
	}

	// Reset
	SetLocomotionStatus(ELocomotionStatus::MAX);
}

UAnimMontage* UVMM_GameplayAbility_Locomotion::GetLocomotionMontage(const ELocomotionStatus* InStatus) const
{
	// Choose cached locomotion status
	switch (InStatus ? *InStatus : LocomotionStatus)
	{
	case ELocomotionStatus::TURNINPLACE:
		return TurnMontage;

	case ELocomotionStatus::START:
		return StartMontage;

	case ELocomotionStatus::MOVING:
		return MovingMontage;

	case ELocomotionStatus::PIVOT:
		return PivotMontage;

	case ELocomotionStatus::STOP:
	case ELocomotionStatus::MAX:
		return StopMontage;
	}

	// INVALID
	return nullptr;
}

const FLocomotionParams* UVMM_GameplayAbility_Locomotion::GetLocomotionParams() const
{
	// Check the data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::GetLocomotionParams, Invalid data asset, params index: %d"), int32(SkillParamsGroup.ParamsIndex));
		return nullptr;
	}

	// Return result
	return LocomotionData->GetLocomotionParamsConst(SkillParamsGroup.ParamsIndex);
}
#pragma endregion


#pragma region Input
EAngleDirection UVMM_GameplayAbility_Locomotion::GetDesiredInputDirection(FRotator* OutRotation) const
{
	// Define the default foot direction type
	EAngleDirection theFootDirection = EAngleDirection::MAX;

	// Check the ability system component is valid
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent == nullptr)
	{
		return theFootDirection;
	}

	// Check the avatar actor is valid
	AVMM_Character* theAvatarCharacter = Cast<AVMM_Character>(theAbilityComponent->GetAvatarActor());
	if (theAvatarCharacter == nullptr)
	{
		return theFootDirection;
	}

	// Get actor view rotation data
	FVector theEyeLoc; FRotator theEyeRot;
	theAvatarCharacter->GetActorEyesViewPoint(theEyeLoc, theEyeRot);

	// Get the avatar rotation
	const FRotator theAvatarRotation = theAvatarCharacter->GetActorRotation();

	// Get the input rotation
	FRotator theInputRotation = FRotator::ZeroRotator;
	if (theAvatarCharacter && theAvatarCharacter->IsVelocityRotationMode())
	{
		if (UVMM_CharacterMovementComponent* theMovementComp = theAvatarCharacter->CharacterMovementComponent)
		{
			theInputRotation = theMovementComp->GetAdjustMovementRotation();
		}
	}

	// Calculate the local view rotation
	FRotator theLocalViewRotation = theEyeRot - theAvatarRotation;
	theLocalViewRotation.Normalize();

	// Calculate the input rotation angle
	const FRotator theDesiredInputRotation = theInputRotation.IsZero() ? theLocalViewRotation : (FQuat(theInputRotation) * FQuat(theLocalViewRotation)).Rotator();

	// Calculate the start direction type
	theFootDirection = UVMM_Library::GetAngleDirectionType(theDesiredInputRotation.Yaw);

	// Output the rotation value
	if (OutRotation != nullptr)
	{
		*OutRotation = theDesiredInputRotation;
	}

	// Return the result
	return theFootDirection;
}

void UVMM_GameplayAbility_Locomotion::ResponseMovementDirection(UVMM_CharacterMovementComponent* InMovementComp)
{
	check(InMovementComp);

	// Check the response condition
	if (CanResponseMovement() == false)
	{
		return;
	}

	// Check the ability system component is valid
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent == nullptr)
	{
		return;
	}

	// Check the movement mode is valid
	if (!InMovementComp->IsMovingOnGround())
	{
		return;
	}

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		//UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementDirection, Invalid locomotion data %s."), *InMovementComp->GetCharacterOwner()->GetName());
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementDirection, Invalid locomotion data."));
		return;
	}

	// Get current movement data
	const FIntPoint theGaitAxisRange = InMovementComp->GetGaitAxisRange();
	const EAngleDirection theMoveFootDirection = GetDesiredInputDirection();
	const ECardinalDirection theMoveDirection = InMovementComp->GetAdjustMovementCardinalDirection();
	const ECardinalDirection& theLastMoveDirection = InMovementComp->GetAdjustLastMovementCardinalDirection();

	// Check the movement input condition
	if (theMoveDirection == ECardinalDirection::MAX)
	{
		UE_LOG(LogVirtualMotionMatching, Verbose, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementDirection, StopInput: %d"), GFrameCounter);

		// Calculate delta time from two movement input
		const double theDeltaInputTime = FPlatformTime::Seconds() - LastResponseMovementInputTime;

		// Check start state
		const bool bIsShortStarting = theDeltaInputTime <= 0.11f;

		// Check stop state
		const bool bIsStopping = LocomotionStatus == ELocomotionStatus::STOP || LocomotionStatus == ELocomotionStatus::MAX;	

		// If the locomotion status is idle/short start/stop, we should try play turn in place animation
		if (bIsShortStarting || bIsStopping)
		{
			if (ResponseTurnInPlace(theGaitAxisRange, theMoveFootDirection))
			{
				return;
			}
		}

		// If is not moving status, we don't play stop.
		if (LocomotionStatus < ELocomotionStatus::START || LocomotionStatus > ELocomotionStatus::PIVOT)
		{
			return;
		}

		// If response failed, we should stop locomotion montage group
		if (ResponseMovementStop(theGaitAxisRange, theLastMoveDirection) == false)
		{
			// Stop the active locomotion montage
			StopSkillMontage(this, GetCurrentActivationInfo(), GetLocomotionMontage());

			// Reset locomotion status
			SetLocomotionStatus(ELocomotionStatus::MAX);
		}
	}
	else if(InMovementComp->HasMovementInput())
	{
		UE_LOG(LogVirtualMotionMatching, Verbose, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementDirection, StartInput: %d"), GFrameCounter);

		// Cache movement input time
		LastResponseMovementInputTime = FPlatformTime::Seconds();

		// Set rotation rate
		InMovementComp->RotationRate.Yaw = LocomotionData->GetCircleIncrementalAngle(theGaitAxisRange, InMovementComp->GetRotationMode(), theMoveDirection);

		// If is transition status, we should check the play condition
		if (LocomotionStatus == ELocomotionStatus::TRANSITION)
		{}

		// If is stop status, we should check the play condition
		if (LocomotionStatus == ELocomotionStatus::STOP)
		{
			// If we still keep a higher weighted moving animation, we should respond to the pivot in time
			UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo();
			if (FAnimMontageInstance* theMovingMontageInstance = UVMM_MontageLibrary::GetMontageInstance(theAnimInstance, MovingMontage))
			{
				const float theMontageWeight = theMovingMontageInstance->GetWeight();
				if (theMontageWeight >= LocomotionData->ResponsePivotAsMovingWeightFromStop || theMovingMontageInstance->GetBlendTime() <= LocomotionData->ResponsePivotAsMovingBlendTimeFromStop)
				{
					// Reset the last move direction

					// Update response movement pivot animation
					if (UpdateMovementPivot(0.f, &StopMontageDirection))
					{
						return;
					}
				}
			}
		}

		// Handle movement event
		switch (LocomotionStatus)
		{
		case ELocomotionStatus::MAX:
		case ELocomotionStatus::STOP:
		case ELocomotionStatus::TRANSITION:
		case ELocomotionStatus::TURNINPLACE:
			// We always try play start animation
			if (ResponseMovementStart(theGaitAxisRange, theMoveFootDirection, theMoveDirection))
			{
				break;
			}

			// We always try play default moving animation
			ResponseMovementLoop(theGaitAxisRange, theMoveDirection);
			break;

		case ELocomotionStatus::START:
			// Update response movement pivot animation
			if (UpdateMovementPivot())
			{
				return;
			}
			
			// If is new direction, we can fast blend it.
			if (StartMontageDirection != theMoveDirection)
			{
				// Stop the active locomotion montage
				StopSkillMontage(this, GetCurrentActivationInfo(), GetLocomotionMontage());
			}
			break;

		case ELocomotionStatus::MOVING:
			// Update response movement pivot animation
			if (UpdateMovementPivot())
			{
				return;
			}

			// Always response movement loop
			ResponseMovementLoop(theGaitAxisRange, theMoveDirection);
			break;
		}
	}
}

void UVMM_GameplayAbility_Locomotion::ResponseMovementAbility(UVMM_CharacterMovementComponent* InMovementComp, bool InTrigger)
{
	if (InTrigger)
	{
		// Check the response movement condition
		if (CanResponseMovement() == false)
		{
			//CanActivateAbility()
			return;
		}

		// Check the movement component is valid
		UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
		if (theMovementComp == nullptr)
		{
			return;
		}

		// Trigger moving on ground mode
		if (!theMovementComp->IsMovingOnGround())
		{
			return;
		}

		// If is crouched, we should execute disable crouch, don't play jump.
		if (theMovementComp->IsCrouching())
		{
			if (ACharacter* theCharacterOwner = theMovementComp->GetCharacterOwner())
			{
				theCharacterOwner->UnCrouch();
			}
			return;
		}

		// Response jump only --- NOW
		ResponseMovementJump(InMovementComp, ELocomotionStatus::JUMPSTART);
	}
}
#pragma endregion


#pragma region Movement
void UVMM_GameplayAbility_Locomotion::OnJumpApexCallback()
{
	// If is playing falling montage, we don't play again
	if (UVMM_MontageLibrary::GetMontageIsPlaying(GetAnimInstanceFromActorInfo(), FallingMontage))
	{
		return;
	}

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		return;
	}

	// Response play falling animation
	ResponseMovementJump(theMovementComp, ELocomotionStatus::JUMPFALLING);
}

void UVMM_GameplayAbility_Locomotion::OnLandedCallback(const FHitResult& Hit)
{
	ResponseMovementJump(GetVirtualCharacterMovementComponentFromActorInfo(), ELocomotionStatus::JUMPLAND);
}

void UVMM_GameplayAbility_Locomotion::OnFaceRotation(const float& InDeltaSeconds, const FRotator& InControlRotation)
{
	// Check the response condition
	if (CanResponseMovement() == false)
	{
		return;
	}

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		return;
	}

	// Check the movement mode is valid
	if (!theMovementComp->IsMovingOnGround())
	{
		return;
	}

	// Update response movement pivot animation
	if (UpdateMovementPivot(InDeltaSeconds))
	{
		return;
	}

	// Update turn in place
	if (UpdateTurnInPlace(InDeltaSeconds) == false)
	{
		// We should try update turn rate
	}
}

void UVMM_GameplayAbility_Locomotion::OnRotationModeChanged(const ERotationMode& InRotationMode, const ERotationMode& InLastRotationMode, const ERotationMode& InLastLookingRotationModee)
{
	// We always adjust new locomotion data
	if (UVMM_AnimInstance* theCustomAnimInstance = Cast<UVMM_AnimInstance>(GetAnimInstanceFromActorInfo()))
	{
		if (theCustomAnimInstance->UpdateLocomotionData())
		{
			// If has update locomotion data, we should wait OnLocomotionDataChanged event.
			return;
		}
	}

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		return;
	}

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::SetLocomotionData, Invalid movement component"));
		return;
	}

	// Get current movement data
	const FIntPoint theGaitAxisRange = theMovementComp->GetGaitAxisRange();
	const EAngleDirection theMoveFootDirection = GetDesiredInputDirection();
	const ECardinalDirection theMoveDirection = theMovementComp->GetAdjustMovementCardinalDirection();
	const ECardinalDirection theLastMoveDirection = theMovementComp->GetMovementCardinalDirection();

	// Set rotation rate
	theMovementComp->RotationRate.Yaw = LocomotionData->GetCircleIncrementalAngle(theGaitAxisRange, theMovementComp->GetRotationMode(), theMoveDirection);

	// Response moving on ground now
	if (!theMovementComp->IsMovingOnGround() || !theMovementComp->HasMovementInput())
	{
		return;
	}

	// Check the new movement direction is valid
	if (theMoveDirection == ECardinalDirection::Forward && theLastMoveDirection == ECardinalDirection::Forward
		&& (LocomotionStatus == ELocomotionStatus::START || LocomotionStatus == ELocomotionStatus::MOVING))
	{
		return;
	}

	// Check the avatar character is valid
	AVMM_Character* theAvatarCharacter = Cast<AVMM_Character>(theMovementComp->GetCharacterOwner());
	if (theAvatarCharacter == nullptr || !theAvatarCharacter->IsLocallyControlled())
	{
		return;
	}

	// Clear current montage bind event
	UVMM_MontageLibrary::ClearMontageBindEvent(GetAnimInstanceFromActorInfo(), GetLocomotionMontage());

	// Response movement state again
	if (UpdateMovementPivot() == false)
	{
		// Always check the moving weight is valid
		if (true || theMovementComp->Velocity.Size2D() > 100.f)
		{
			// Response stop... haven't other animation....
			ResponseMovementStop(theGaitAxisRange, InLastRotationMode == ERotationMode::Velocity ? ECardinalDirection::Forward : theLastMoveDirection);

			// Try lock rotation input, can't bank rotation
			if (bLockRotationInput == false)
			{
				bLockRotationInput = true;
				theMovementComp->SetIgnoreRotationInput(true);
			}
		}
		else
		{
			ResponseMovementLoop(theGaitAxisRange, theMoveDirection);
		}
	}
}

void UVMM_GameplayAbility_Locomotion::OnMovementModeChange(ACharacter* InCharacter, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	check(InCharacter);

	// Check the ability system component is valid
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent == nullptr)
	{
		return;
	}

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = Cast<UVMM_CharacterMovementComponent>(InCharacter->GetMovementComponent());
	if (theMovementComp == nullptr)
	{
		return;
	}

	// Get current movement mode data
	const EMovementMode& theMovementMode = theMovementComp->MovementMode;
	
	// Remove the last movement mode tag
	if (const FGameplayTag* theMovementModeTag = MovementModeTags.Find(PrevMovementMode))
	{
		if (theMovementModeTag->IsValid())
		{
			theAbilityComponent->RemoveLooseGameplayTag(*theMovementModeTag);
		}
	}
	else
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::OnMovementModeChange, Invalid last movement mode tag: %d")
			, int32(PrevMovementMode));
	}

	// Add the movement mode tag
	if (const FGameplayTag* theMovementModeTag = MovementModeTags.Find(theMovementMode))
	{
		if (theMovementModeTag->IsValid())
		{
			theAbilityComponent->AddLooseGameplayTag(*theMovementModeTag);
		}
	}
	else
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::OnMovementModeChange, Invalid movement mode tag: %d")
			, int32(theMovementMode));
	}

	// Response falling mode
	if (theMovementMode == EMovementMode::MOVE_Falling)
	{
		// If is playing procedural jump start animation, we should wait jump apex callback.
		if (theMovementComp->bNotifyApex
			&& UVMM_MontageLibrary::GetMontageIsPlaying(GetAnimInstanceFromActorInfo(), JumpMontage) && !JumpMontage->HasRootMotion())
		{
			return;
		}

		// UnCrouch
		InCharacter->UnCrouch();

		// Response play falling animation
		ResponseMovementJump(theMovementComp, ELocomotionStatus::JUMPFALLING);
	}
}
#pragma endregion


#pragma region Gait
bool UVMM_GameplayAbility_Locomotion::AdjustGaitAxis(AVMM_Character* InCharacter)
{
	check(InCharacter);

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		return false;
	}

	// Calculate the max gait axis
	const int32 theMaxGaitAxis = LocomotionData->GaitParams.Num() - 1;

	// Get current gait axis
	const uint8 theGaitAxis = InCharacter->GetGaitAxis();

	// Reset the gait axis
	if (theGaitAxis > theMaxGaitAxis)
	{
		InCharacter->OnGaitAxisChanged(theMaxGaitAxis);
	}
	else
	{
		InCharacter->OnPressedGaitAxis();
	}

	// Return the result
	return true;
}

void UVMM_GameplayAbility_Locomotion::OnGaitChanged(AVMM_Character* InCharacter)
{
	check(InCharacter);

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		return;
	}

	// Check the character owner is valid
	if (InCharacter == nullptr || !InCharacter->IsLocallyControlled())
	{
		return;
	}

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_Locomotion::OnGaitChanged, Invalid movement component"));
		return;
	}

	// Get current movement data
	const uint8& theLastGaitAxis = InCharacter->GetLastGaitAxis();
	const FIntPoint theGaitAxisRange = theMovementComp->GetGaitAxisRange();
	const EAngleDirection theMoveFootDirection = GetDesiredInputDirection();
	const ECardinalDirection theMoveDirection = theMovementComp->GetAdjustMovementCardinalDirection();

	// Check the gait axis is valid
	if (theGaitAxisRange.X > LocomotionData->GaitParams.Num() - 1)
	{
		return;
	}

	// Set rotation rate
	theMovementComp->RotationRate.Yaw = LocomotionData->GetCircleIncrementalAngle(theGaitAxisRange, theMovementComp->GetRotationMode(), theMoveDirection);
	
	// Response moving on ground now
	if (!theMovementComp->IsMovingOnGround() || !theMovementComp->HasMovementInput())
	{
		return;
	}

	UE_LOG(LogVirtualMotionMatching, Verbose, TEXT("UVMM_GameplayAbility_Locomotion::OnGaitChanged: %d"), GFrameCounter);

	// Get the gait attribute data
	const FLocomotionGaitAttributeData* theGaitAttributeData = LocomotionData->GetGaitAttributeData(theGaitAxisRange.X, theLastGaitAxis);
	if (theGaitAttributeData)
	{
		// Gait up
		if (theGaitAxisRange.X > theLastGaitAxis)
		{
			if (theGaitAttributeData->bResponseMovementStartInUp)
			{
				ResponseMovementStart(theGaitAxisRange, theMoveFootDirection, theMoveDirection);
				return;
			}
			else if (theGaitAttributeData->bResponseMovementStartInDown)
			{
				ResponseMovementStop(FIntPoint(theLastGaitAxis,theGaitAxisRange.Y), theMoveDirection);
				return;
			}
		}
		else
		{
			if (theGaitAttributeData->bResponseMovementStartInDown)
			{
				ResponseMovementStart(theGaitAxisRange, theMoveFootDirection, theMoveDirection);
				return;
			}
			else if (theGaitAttributeData->bResponseMovementStopInDown)
			{
				ResponseMovementStop(FIntPoint(theLastGaitAxis, theGaitAxisRange.Y), theMoveDirection);
				return;
			}
		}
	}

	// Response movement state again
	switch (LocomotionStatus)
	{
	case ELocomotionStatus::START:
#if 1
		// Stop the active locomotion montage
		StopSkillMontage(this, GetCurrentActivationInfo(), GetLocomotionMontage());
#else
		ResponseMovementStart(theGaitAxisRange, theMoveFootDirection, theMoveDirection);
#endif
		return;

	case ELocomotionStatus::MOVING:
		ResponseMovementLoop(theGaitAxisRange, theMoveDirection);
		return;
	}
}
#pragma endregion


#pragma region Transition
bool UVMM_GameplayAbility_Locomotion::ResponseDataTransition()
{
	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo();
	if (theAnimInstance == nullptr)
	{
		return false;
	}

	// Local function
	auto OnFailed = [&]()
	{
		// Revert the before animation layer weight
		//theAnimInstance->RevertBeforeLayerWeight();
	};

	// Check the last locomotion data is valid
	if (LastLocomotionData == nullptr)
	{
		OnFailed();
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseDataTransition, Invalid last locomotion data."));
		return false;
	}

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		OnFailed();
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseDataTransition, Invalid locomotion data."));
		return false;
	}

	// Check the transition animations data is valid
	const FLocomotionTransitionAnims* theTransitionParams = LastLocomotionData->GetTransitionAnimParams(0, &LocomotionData->GetDataName());
	if (theTransitionParams == nullptr)
	{
		OnFailed();
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseDataTransition, Invalid transition params, Last: %s, Current: %s")
			, *LastLocomotionData->GetDataName().ToString(), *LocomotionData->GetDataName().ToString());
		return false;
	}

	// Check the params is valid
	const FLocomotionParams* theLocomotionParams = theTransitionParams->MultiParams.Num() > 0 ? &theTransitionParams->MultiParams[0] : nullptr;
	if (theLocomotionParams == nullptr)
	{
		OnFailed();
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseDataTransition, Invalid transition params, Last: %s, Current: %s")
			, *LastLocomotionData->GetDataName().ToString(), *LocomotionData->GetDataName().ToString());
		return false;
	}

	// Define the skill params group data
	const FVirtualSkillParamsGroup theSkillParamsGroup(LastLocomotionData->GetDataIndex(), theLocomotionParams->Index);

	// Calculate the start position
	float thePlayRate = theLocomotionParams->PlayRate;
	float theStartPosition = theLocomotionParams->StartPosition;

	// Make a dynamic montage
	UAnimMontage* thePlayMontage = UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage(theAnimInstance, theLocomotionParams);
	if (thePlayMontage == nullptr)
	{
		OnFailed();
		return false;
	}

	// Play the skill params
	const bool bIsPlaying = PlaySkillMontage(theSkillParamsGroup, thePlayMontage, thePlayRate, theStartPosition);
	if (bIsPlaying == false)
	{
		OnFailed();
		return false;
	}

	// If is playing the params montage, we should cache the params data
	TransitionMontage = thePlayMontage;
	SetLocomotionStatus(theLocomotionParams->LocomotionStatus);

	// Revert the before animation layer weight
	if (UVMM_AnimInstance* theCustomAnimInstance = Cast<UVMM_AnimInstance>(theAnimInstance))
	{
		theCustomAnimInstance->RevertBeforeLayerWeight();
	}

	// Return the result
	return bIsPlaying;
}
#pragma endregion


#pragma region Turn
bool UVMM_GameplayAbility_Locomotion::CanChangeTurn(const UVMM_LocomotionData* InData, const FLocomotionParams* InParams, bool* OutResult /*= nullptr*/, float* OutBlendTime /*= nullptr*/)
{
	// Check the ability system component is valid
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent == nullptr)
	{
		return false;
	}

	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = theAbilityComponent->GetAnimInstance();
	if (theAnimInstance == nullptr)
	{
		return false;
	}

	// Check last montage instance isvalid
	FAnimMontageInstance* theLastMontageInstance = UVMM_MontageLibrary::GetMontageInstance(theAnimInstance, TurnMontage);
	if (theLastMontageInstance == nullptr)
	{
		return true;
	}

	// Calculate the montage play state
	if (!theLastMontageInstance->IsPlaying())
	{
		return true;
	}

#if 1
	// Check the global setting is valid
	UVMM_Settings* theGlobalSettings = GetMutableDefault<UVMM_Settings>();
	if (theGlobalSettings == nullptr)
	{
		return false;
	}

	// Check the rotation curve name is valid
	const FName* theRotationCurveName = theGlobalSettings->GetMotionCurveName(EMotionCurveType::YawRotationAlpha);
	if (theRotationCurveName == nullptr)
	{
		return false;
	}

	// Search the curve data
	bool bHasCurveData = false;
	const float theCurveValue = UVMM_CurveLibrary::GetCurveValueFromMontage(TurnMontage, *theRotationCurveName, theLastMontageInstance->GetPosition(), bHasCurveData);

	// Check the curve data is valid
	if (bHasCurveData == false)
	{
		return false;
	}

	// Check the rotation alpha is full weight
	if (theCurveValue > 0.975f)
	{
		return true;
	}
#endif

	// Check the params data is valid
	if (SkillParamsGroup.DataIndex == InData->GetDataIndex() && SkillParamsGroup.ParamsIndex == InParams->Index)
	{
		return false;
	}

	// Check the cached turn angle is valid
	if (TurnAngle != 0.f)
	{
		// Get the desired turn angle
		const float theDesiredAngle = InParams->AnimatedRotation.Yaw;
		if (theDesiredAngle != 0.f)
		{
			// Check the fast path rotate axis is valid
			const float theLastAxis = TurnAngle / FMath::Abs(TurnAngle);
			const float theDeisredAxis = theDesiredAngle / FMath::Abs(theDesiredAngle);
			if (theLastAxis == theDeisredAxis)
			{
				return false;
			}
		}
	}

	// Return the result
	return true;
}

bool UVMM_GameplayAbility_Locomotion::UpdateTurnInPlace(const float InDeltaSeconds)
{
	// If the trajectory status is pivot, we don't check again
	if (TrajectoryLocomotionStatus.IsSet() && TrajectoryLocomotionStatus.GetValue() == ELocomotionStatus::TURNINPLACE)
	{
		return false;
	}

	// Check the status is valid
	if (LocomotionStatus >= ELocomotionStatus::START && LocomotionStatus <= ELocomotionStatus::PIVOT)
	{
		return false;
	}

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::UpdateMovementPivot, Invalid locomotion data."));
		return false;
	}

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		return false;
	}

	// Check the movement input state
	if (theMovementComp->HasPlayerInput())
	{
		return false;
	}

	// Check the avatar character is valid
	AVMM_Character* theAvatarCharacter = Cast<AVMM_Character>(theMovementComp->GetCharacterOwner());
	if (theAvatarCharacter == nullptr || !theAvatarCharacter->IsLocallyControlled())
	{
		return false;
	}

	// Check the response turn in place condition
	const bool bCanResponseTurnInPlace = theAvatarCharacter->IsLookingRotationMode() || !theAvatarCharacter->IsPlayerControlled();
	if (!bCanResponseTurnInPlace)
	{
		return false;
	}

	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo();
	if (theAnimInstance == nullptr)
	{
		return false;
	}

#if 0
	// Check the montage play condition
	if (UVMM_MontageLibrary::GetMontageIsPlaying(theAnimInstance, PivotMontage))
	{
		return false;
	}
#endif

#if 1 // Waiting task
	// Get response time from locomotion data
	const float& theWaitTime = LocomotionData->GetResponseTurnInPlaceIntervalTime();

	// Response pivot event
	if (UVMM_Library::WaitTime(InDeltaSeconds, ResponseTurnAccumulateTime, theWaitTime))
	{
		ResponseTurnAccumulateTime = 0.f;
	}
	else
	{
		return false;
	}
#endif

	// Get current movement data
	FRotator theDesiredRotation;
	const FIntPoint theGaitAxisRange = theMovementComp->GetGaitAxisRange();
	const EAngleDirection theMoveFootDirection = GetDesiredInputDirection(&theDesiredRotation);

	// Check the response angle is valid
	const FVector2D& theResponseAngleRange = LocomotionData->GetResponseTurnInPlaceAngleRange();
	if (FMath::IsWithinInclusive(theDesiredRotation.Yaw, theResponseAngleRange.X, theResponseAngleRange.Y))
	{
		return false;
	}

	// Return the result
	return ResponseTurnInPlace(theGaitAxisRange, theMoveFootDirection);
}

bool UVMM_GameplayAbility_Locomotion::ResponseTurnInPlace(const FIntPoint& InGatAxisRange, const EAngleDirection& InDirection)
{
	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo();
	if (theAnimInstance == nullptr)
	{
		return false;
	}

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseTurnInPlace, Invalid locomotion data."));
		return false;
	}

	// Check the params is valid
	const FLocomotionParams* theLocomotionParams = LocomotionData->GetTurnInPlaceParams(InGatAxisRange, InDirection);
	if (theLocomotionParams == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseTurnInPlace, Invalid locomotion params, Gait: %s, Direction: %s")
			, *InGatAxisRange.ToString(), *UVMM_Library::GetAngleDirectionString(InDirection));
		return false;
	}

	// Check the response condition
	if (!CanChangeTurn(LocomotionData, theLocomotionParams))
	{
		return false;
	}

	// Define the skill params group data
	const FVirtualSkillParamsGroup theSkillParamsGroup(LocomotionData->GetDataIndex(), theLocomotionParams->Index);

	// Calculate the start position
	float thePlayRate = theLocomotionParams->PlayRate;
	float theStartPosition = theLocomotionParams->StartPosition;

	// Make a dynamic montage
	UAnimMontage* thePlayMontage = UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage(theAnimInstance, theLocomotionParams);
	if (thePlayMontage == nullptr)
	{
		return false;
	}

	// Cache last locomotion montage instance
	TArray<FAnimMontageInstance*> theLastMotionMontageInstances;

	// Each every montage instance
	for (FAnimMontageInstance* theMontageInstance : theAnimInstance->MontageInstances)
	{
		// Check the montage instance is valid
		if (theMontageInstance == nullptr)
		{
			continue;
		}

		// Check is equal desired montage
		if (theMontageInstance->Montage == StartMontage || theMontageInstance->Montage == TurnMontage)
		{
			theLastMotionMontageInstances.Add(theMontageInstance);
		}

		// Always set the turn blend in time
		if (thePlayMontage->BlendIn.GetBlendTime() != 0.4f)
		{
			thePlayMontage->BlendIn.SetBlendTime(0.4f);
			thePlayMontage->BlendIn.SetBlendOption(EAlphaBlendOption::HermiteCubic);
			thePlayMontage->BlendIn.Reset();
		}
	}

	// Play the skill params
	const bool bIsPlaying = PlaySkillMontage(theSkillParamsGroup, thePlayMontage, thePlayRate, theStartPosition);
	if (bIsPlaying == false)
	{
		return false;
	}

	// Pause every locomotion montage instance
	for (FAnimMontageInstance* theMontageInstance : theLastMotionMontageInstances)
	{
		// Check the montage instance is valid
		if (theMontageInstance == nullptr)
		{
			continue;
		}

		// Pause
		theMontageInstance->Pause();
	}

	// If is playing the params montage, we should cache the params data
	TurnMontage = thePlayMontage;
	TurnAngle = theLocomotionParams->AnimatedRotation.Yaw;
	SetLocomotionStatus(theLocomotionParams->LocomotionStatus);

	// Return the result
	return bIsPlaying;
}
#pragma endregion


#pragma region Start
bool UVMM_GameplayAbility_Locomotion::ResponseMovementStart(const FIntPoint& InGatAxisRange
	, const EAngleDirection& InFaceDirection, const ECardinalDirection& InDirection)
{
	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementStart, Invalid locomotion data."));
		return false;
	}

	// Check the enable condition
	if (!LocomotionData->CanResponseStartStep())
	{
		return false;
	}

	// Check the params is valid
	const FLocomotionParams* theLocomotionParams = LocomotionData->GetStartParams(InFaceDirection, InDirection, InGatAxisRange);
	if (theLocomotionParams == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementStart, Invalid locomotion params, Gait: %s, Face: %s, Direction: %s")
			, *InGatAxisRange.ToString(), *UVMM_Library::GetAngleDirectionString(InFaceDirection), *UVMM_Library::GetCardinalDirectionString(InDirection));
		return false;
	}

	// Define the skill params group data
	const FVirtualSkillParamsGroup theSkillParamsGroup(LocomotionData->GetDataIndex(), theLocomotionParams->Index);

	// Calculate the start position
	float thePlayRate = theLocomotionParams->PlayRate;
	float theStartPosition = theLocomotionParams->StartPosition;

	// Trajectory status
	TrajectoryGaitAxis = InGatAxisRange.X;
	TrajectoryLocomotionStatus = ELocomotionStatus::START;

	// Always clear same status bind event
	if (true/*LocomotionStatus == ELocomotionStatus::START*/)
	{
		UVMM_MontageLibrary::ClearMontageBindEvent(GetAnimInstanceFromActorInfo(), StartMontage);
	}

	UE_LOG(LogVirtualMotionMatching, Verbose, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementStart-Begin: %d"), GFrameCounter);

	// Play the skill params
	UAnimMontage* thePlayMontage = nullptr;
	const bool bIsPlaying = PlaySkillMontage(theSkillParamsGroup, thePlayMontage, thePlayRate, theStartPosition);
	if (bIsPlaying == false)
	{
		// Clear invalid play state.
		TrajectoryGaitAxis.Reset();
		TrajectoryLocomotionStatus.Reset();
		return false;
	}

	UE_LOG(LogVirtualMotionMatching, Verbose, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementStart-End: %d, Montage: %s")
		, GFrameCounter, *GetNameSafe(thePlayMontage));

	// Response montage blend out now...so should clear it.
	TrajectoryGaitAxis.Reset();
	TrajectoryLocomotionStatus.Reset();

	// If is playing the params montage, we should cache the params data
	StartMontage = thePlayMontage;
	StartMontageDirection = theLocomotionParams->CardinalDirection;
	SetLocomotionStatus(theLocomotionParams->LocomotionStatus);

	// If the params has animated rotation, we should lock rotation input
	if (theLocomotionParams->HasRotation())
	{
		if (UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo())
		{
			bLockRotationInput = true;
			theMovementComp->SetIgnoreRotationInput(true);
		}
	}

	// Since the engine cannot be modified, we can only try to modify the hybrid mode in a stand-alone situation
	if (UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo())
	{
		if (AActor* thePawnOwner = theAnimInstance->TryGetPawnOwner())
		{
			if (thePawnOwner->IsNetMode(NM_Standalone) || true)
			{
				theAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
			}
		}
	}

	// Return the result
	return bIsPlaying;
}
#pragma endregion


#pragma region Moving
bool UVMM_GameplayAbility_Locomotion::CanResponseChangeMoving(const UVMM_LocomotionData* InData
	, const FLocomotionParams* InParams, bool* OutResult, float* OutBlendTime)
{
	// Check the ability system component is valid
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent == nullptr)
	{
		return false;
	}

	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = theAbilityComponent->GetAnimInstance();
	if (theAnimInstance == nullptr)
	{
		return false;
	}

#if 1
	// Define the moving montages number
	int32 theMovingMontageNumber = 0;

	// Each every montage instance
	for (FAnimMontageInstance* theMontageInstance : theAnimInstance->MontageInstances)
	{
		// Check the montage instance is valid
		if (theMontageInstance == nullptr)
		{
			return nullptr;
		}

		// Check is equal desired montage
		if (RecordMovingMontages.Num() > 0 && RecordMovingMontages.Contains(theMontageInstance->Montage))
		{
			++theMovingMontageNumber;
		}

		// We allow max moving montage number is 3
		if (theMovingMontageNumber >= 3)
		{
			return false;
		}
	}
#else
	// Calculate the locomotion montage play state
	const bool bIsPlayingMontage = UVMM_MontageLibrary::GetMontageIsPlaying(theAnimInstance, MovingMontage);
	if (bIsPlayingMontage == false)
	{
		return true;
	}
#endif

	// Check the params data is valid
	if (SkillParamsGroup.DataIndex == InData->GetDataIndex() && SkillParamsGroup.ParamsIndex == InParams->Index)
	{
		return false;
	}

	// Return the result
	return true;
}

bool UVMM_GameplayAbility_Locomotion::ResponseMovementLoop(const FIntPoint& InGatAxisRange, const ECardinalDirection& InDirection)
{
	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo();
	if (theAnimInstance == nullptr)
	{
		return false;
	}

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementLoop, Invalid locomotion data."));
		return false;
	}

	// Check the params is valid
	const FLocomotionParams* theLocomotionParams = LocomotionData->GetMovingParams(InGatAxisRange, InDirection);
	if (theLocomotionParams == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementLoop, Invalid locomotion params, Gait: %s, Direction: %s")
			, *InGatAxisRange.ToString(), *UVMM_Library::GetCardinalDirectionString(InDirection));
		return false;
	}

	// Check the response condition
	if (!CanResponseChangeMoving(LocomotionData, theLocomotionParams))
	{
		return false;
	}

	// Define the skill params group data
	const FVirtualSkillParamsGroup theSkillParamsGroup(LocomotionData->GetDataIndex(), theLocomotionParams->Index);
	
	// Calculate the start position
	float thePlayRate = theLocomotionParams->PlayRate;
	float theStartPosition = theLocomotionParams->StartPosition;

	// Make a dynamic montage
	UAnimMontage* thePlayMontage = UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage(theAnimInstance, theLocomotionParams);

	// Try apply pose search
	if (LocomotionStatus == ELocomotionStatus::START || LocomotionStatus == ELocomotionStatus::MOVING || LocomotionStatus == ELocomotionStatus::PIVOT)
	{
		if (UAnimMontage* theLastMontage = GetLocomotionMontage())
		{
			// Check the skill data asset is valid
			UVMM_Settings* theGlobalManager = GetMutableDefault<UVMM_Settings>();
			const FLocomotionParams* theLastLocomotionParams = theGlobalManager ? theGlobalManager->GetParams(SkillParamsGroup) : nullptr;

			// Matching the montage blend remaining time - Check data index, gait axis etc.
			if (SkillParamsGroup.DataIndex == LocomotionData->GetDataIndex())
			{
				// Don't allow moving status and different gait axis
				if (LocomotionStatus != ELocomotionStatus::MOVING
					&& theLastLocomotionParams != nullptr
					&& theLocomotionParams->GaitAxis == theLastLocomotionParams->GaitAxis)
				{
					UVMM_MontageLibrary::MatchingMontageBlendRemainingTime(theAnimInstance, theLastMontage, thePlayMontage);
				}

// 				if (theLastLocomotionParams->LocomotionStatus == ELocomotionStatus::MOVING)
// 				{
// 				UVMM_MontageLibrary::GetMontageBlendWeight()
// 				}	
			}

			// Get last montage play position
			const float theLastPosition = UVMM_MontageLibrary::GetMontagePlayPosition(GetAnimInstanceFromActorInfo(), theLastMontage, false);

			// If is same sync group, we should use time matching
			if (theLocomotionParams->SyncGroup.GroupName != NAME_None && theLastLocomotionParams && theLastLocomotionParams->SyncGroup.GroupName == theLocomotionParams->SyncGroup.GroupName)
			{
				theStartPosition = theLastPosition;
			}
			else
			{
				// Compare to poses
				theStartPosition = UVMM_PoseSearchLibrary::ComparePosesAsMontage(theLastPosition, theLastMontage, thePlayMontage
					, LocomotionData->GetFootPositionCurveName(), LocomotionData->GetPoseDistanceCurvesNameMap());
			}

			/*UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementLoop, PoseMatching: %f, LastPos: %f, LastMontage: %s, NextMontage: %s")
				, theStartPosition, theLastPosition, *theLastMontage->GetName(), *thePlayMontage->GetName());*/
		}
	}

	// Trajectory status
	TrajectoryGaitAxis = InGatAxisRange.X;
	TrajectoryLocomotionStatus = ELocomotionStatus::MOVING;

	// Always clear same status bind event
	UVMM_MontageLibrary::ClearMontageBindEvent(GetAnimInstanceFromActorInfo(), MovingMontage);

	// Always unlock cached rotation input
	if (UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo())
	{
		bLockRotationInput = false;
		theMovementComp->ResetIgnoreRotationUpdated();
	}

	UE_LOG(LogVirtualMotionMatching, Verbose, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementLoop-Begin: %d"), GFrameCounter);

	// Play the skill params
	const bool bIsPlaying = PlaySkillMontage(theSkillParamsGroup, thePlayMontage, thePlayRate, theStartPosition);
	if (bIsPlaying == false)
	{
		// Clear invalid play state.
		TrajectoryGaitAxis.Reset();
		TrajectoryLocomotionStatus.Reset();
		return false;
	}

	UE_LOG(LogVirtualMotionMatching, Verbose, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementLoop-End: %d, Montage: %s")
		, GFrameCounter, *GetNameSafe(thePlayMontage));

	// Response montage blend out now...so should clear it.
	TrajectoryGaitAxis.Reset();
	TrajectoryLocomotionStatus.Reset();

	// If is playing the params montage, we should cache the params data
	MovingMontage = thePlayMontage;
	SetLocomotionStatus(theLocomotionParams->LocomotionStatus);

	// Cache record moving montages
	RecordMovingMontages.Add(thePlayMontage);

	// Always keep max record number
	if (RecordMovingMontages.Num() > 3)
	{
		RecordMovingMontages.RemoveAt(0);
	}

	// Since the engine cannot be modified, we can only try to modify the hybrid mode in a stand-alone situation
	if (AActor* thePawnOwner = theAnimInstance->TryGetPawnOwner())
	{
		if (thePawnOwner->IsNetMode(NM_Standalone) || true)
		{
			theAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);
		}
	}

	// Return the result
	return bIsPlaying;
}
#pragma endregion


#pragma region Pivot
bool UVMM_GameplayAbility_Locomotion::UpdateMovementPivot(const float InDeltaSeconds, ECardinalDirection* InLastDirection)
{
	// If the trajectory status is pivot, we don't check again
	if (TrajectoryLocomotionStatus.IsSet() && TrajectoryLocomotionStatus.GetValue() == ELocomotionStatus::PIVOT)
	{
		return false;
	}

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::UpdateMovementPivot, Invalid locomotion data."));
		return false;
	}

	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		return false;
	}

	// Check the movement input state
	if (!theMovementComp->HasMovementInput())
	{
		return false;
	}

	// Check the avatar character is valid
	AVMM_Character* theAvatarCharacter = Cast<AVMM_Character>(theMovementComp->GetCharacterOwner());
	if (theAvatarCharacter == nullptr || !theAvatarCharacter->IsLocallyControlled())
	{
		return false;
	}

	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo();
	if (theAnimInstance == nullptr)
	{
		return false;
	}

#if 0
	// Check the montage play condition
	if (UVMM_MontageLibrary::GetMontageIsPlaying(theAnimInstance, PivotMontage))
	{
		return false;
	}
#endif

	// Get response time from locomotion data
	const float& theWaitTime = LocomotionData->GetResponsePivotIntervalTime();

#if 0 // Waiting task
	// Response pivot event
	if (UVMM_Library::WaitTime(InDeltaSeconds, ResponsePivotAccumulateTime, theWaitTime))
	{
		ResponsePivotAccumulateTime = 0.f;
	}
	else
	{
		return false;
	}
#else
	// Calculate delta time from two movement pivot
	const double theDeltaInputTime = FPlatformTime::Seconds() - ResponsePivotTime;
	if (theDeltaInputTime < 0.4f)
	{
		return false;
	}
#endif

	// Get desired rotation and foot direction
	FRotator theDesiredRotation = FRotator::ZeroRotator;
	EAngleDirection theMoveFootDirection = GetDesiredInputDirection(&theDesiredRotation);

	// Get current movement data
	const FIntPoint theGaitAxisRange = theMovementComp->GetGaitAxisRange();
	ECardinalDirection theMoveDirection = theMovementComp->GetAdjustMovementCardinalDirection();
	ECardinalDirection theLastMoveDirection = theMovementComp->GetLastMovementCardinalDirection();

	// If is moving status, we always allow fast path play
	if (LocomotionStatus == ELocomotionStatus::MOVING || LocomotionStatus == ELocomotionStatus::STOP)
	{
		// Check the rotation mode
		if (theAvatarCharacter->IsVelocityRotationMode()
			|| theMoveDirection == ECardinalDirection::Forward)
		{
			// Check the response angle is valid
			const float& theResponseAngle = LocomotionData->GetResponsePivotAngle();
			const bool bResponsePivot = FMath::Abs(theDesiredRotation.Yaw) >= theResponseAngle;
			if (!bResponsePivot && theAvatarCharacter->IsVelocityRotationMode())
			{
				return false;
			}

			// Response movement pivot
			if (bResponsePivot)
			{
				return ResponseMovementPivot(theGaitAxisRange, theMoveFootDirection, theMoveDirection);
			}
		}
		
		// Response looking rotation mode
		if (theAvatarCharacter->IsVelocityRotationMode() == false)
		{
			// Calculate the delta movement rotation
			const FRotator theMovementRotation = theMovementComp->GetMovementRotation();
			const FRotator theLastMovementRotation = theMovementComp->GetLastMovementRotation();
			FRotator theDeltaMovementRotation = theMovementRotation - theLastMovementRotation;
			theDeltaMovementRotation.Normalize();

			// Check the rotation is reverse input?
			if (theDeltaMovementRotation.Yaw < 180.f)
			{
				return false;
			}

			// Check desired last move direction, maybe reset it.....
			if (InLastDirection != nullptr)
			{
				theLastMoveDirection = *InLastDirection;
			}

			// Calculate the delta input rotation
			if (theMoveDirection == theLastMoveDirection
				|| theMoveDirection == ECardinalDirection::MAX
				|| theLastMoveDirection == ECardinalDirection::MAX)
			{
				return false;
			}
			else
			{
				theMoveDirection = theLastMoveDirection;
				theMoveFootDirection = EAngleDirection::MAX;
			}
		}

		// Response movement pivot
		return ResponseMovementPivot(theGaitAxisRange, theMoveFootDirection, theMoveDirection);
	}
	else if (LocomotionStatus == ELocomotionStatus::START || LocomotionStatus == ELocomotionStatus::PIVOT)// Check the rotation input is finished
	{
#if 0
		// Response velocity rotation mode only
		if (!theAvatarCharacter->IsVelocityRotationMode())
		{
			return false;
		}
#endif

		// Check the global setting is valid
		UVMM_Settings* theGlobalSettings = GetMutableDefault<UVMM_Settings>();
		if (theGlobalSettings == nullptr)
		{
			return false;
		}

		// Check the rotation curve name is valid
		const FName* theRotationCurveName = theGlobalSettings->GetMotionCurveName(EMotionCurveType::YawRotationAlpha);
		if (theRotationCurveName == nullptr)
		{
			return false;
		}

		// Get last montage reference
		UAnimMontage* theLastMontage = GetLocomotionMontage();

		// Check last montage instance isvalid
		FAnimMontageInstance* theLastMontageInstance = UVMM_MontageLibrary::GetMontageInstance(theAnimInstance, theLastMontage);
		if (theLastMontageInstance == nullptr)
		{
			return false;
		}
		
		// Simple check the play weight
		if (!theLastMontageInstance->IsStopped() && theLastMontageInstance->GetWeight() != theLastMontageInstance->GetDesiredWeight())
		{
			return false;
		}

		// Simple check the play position
		if (theLastMontageInstance->GetPosition() < (theLastMontage->BlendIn.GetBlendTime() + InDeltaSeconds))
		{
			return false;
		}

		// Search the curve data
		bool bHasCurveData = false;
		const float theCurveValue = UVMM_CurveLibrary::GetCurveValueFromMontage(theLastMontage, *theRotationCurveName, theLastMontageInstance->GetPosition(), bHasCurveData);

		// Check the curve data is valid
		if (bHasCurveData)
		{
			// Check the rotation alpha is full weight
			if (theCurveValue < 0.975f)
			{
				return false;
			}
		}

		// Check the rotation mode
		if (theAvatarCharacter->IsVelocityRotationMode()
			|| theMoveDirection == ECardinalDirection::Forward)
		{
			// Check the response angle is valid
			const float& theResponseAngle = LocomotionData->GetResponsePivotAngle();
			const bool bResponsePivot = FMath::Abs(theDesiredRotation.Yaw) >= theResponseAngle;

			// Always unlock cached rotation input
			if (!bResponsePivot && bLockRotationInput)
			{
				bLockRotationInput = false;
				theMovementComp->SetIgnoreRotationInput(false);
			}

			// Check the condition
			if (!bResponsePivot && theAvatarCharacter->IsVelocityRotationMode())
			{
				return false;
			}

			// Response movement pivot
			if (bResponsePivot)
			{
				return ResponseMovementPivot(theGaitAxisRange, theMoveFootDirection, theMoveDirection);
			}
		}

		// Response looking rotation mode
		if (theAvatarCharacter->IsVelocityRotationMode() == false)
		{
			// Calculate the delta movement rotation
			const FRotator theMovementRotation = theMovementComp->GetMovementRotation();
			const FRotator theLastMovementRotation = theMovementComp->GetLastMovementRotation();
			FRotator theDeltaMovementRotation = theMovementRotation - theLastMovementRotation;
			theDeltaMovementRotation.Normalize();

			// Check the rotation is reverse input?
			if (theDeltaMovementRotation.Yaw < 180.f)
			{
				return false;
			}

			// Calculate the delta input rotation
			if (theMoveDirection == theLastMoveDirection
				|| theMoveDirection == ECardinalDirection::MAX
				|| theLastMoveDirection == ECardinalDirection::MAX)
			{
				return false;
			}
			else
			{
				theMoveDirection = theLastMoveDirection;
				theMoveFootDirection = EAngleDirection::MAX;
			}
		}

		// Response movement pivot
		return ResponseMovementPivot(theGaitAxisRange, theMoveFootDirection, theMoveDirection);
	}

	// Return the result
	return false;
}

bool UVMM_GameplayAbility_Locomotion::ResponseMovementPivot(const FIntPoint& InGatAxisRange, const EAngleDirection& InFaceDirection, const ECardinalDirection& InDirection)
{
	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementPivot, Invalid locomotion data."));
		return false;
	}

	// Check the response pivot condition
	if (!LocomotionData->CanResponsePivotStep(InGatAxisRange.X))
	{
		return false;
	}

	// Calculate current foot position
	const int32 theFootIndex = GetFootPosition();

	// Check the params is valid
	const FLocomotionParams* theLocomotionParams = LocomotionData->GetPivotParams(InFaceDirection, InDirection, InGatAxisRange, theFootIndex);
	if (theLocomotionParams == nullptr)
	{
		/*UE_LOG(LogVirtualMotionMatching, VeryVerbose, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementPivot, Invalid locomotion params, Gait: %s, Face: %s, Direction: %s")
			, *InGatAxisRange.ToString(), *UVMM_Library::GetAngleDirectionString(InFaceDirection), *UVMM_Library::GetCardinalDirectionString(InDirection));*/
		return false;
	}

	// Define the skill params group data
	const FVirtualSkillParamsGroup theSkillParamsGroup(LocomotionData->GetDataIndex(), theLocomotionParams->Index);

	// Calculate the start position
	float thePlayRate = theLocomotionParams->PlayRate;
	float theStartPosition = theLocomotionParams->StartPosition;

	// Trajectory status
	TrajectoryGaitAxis = InGatAxisRange.X;
	TrajectoryLocomotionStatus = ELocomotionStatus::PIVOT;

	// Always clear same status bind event
	if (true/*LocomotionStatus == ELocomotionStatus::PIVOT*/)
	{
		UVMM_MontageLibrary::ClearMontageBindEvent(GetAnimInstanceFromActorInfo(), PivotMontage);
	}

	// If is stop status, we should additive start position - blend out time now.
	if (LocomotionStatus == ELocomotionStatus::STOP)
	{
		if (FAnimMontageInstance* theStopMontageInstance = UVMM_MontageLibrary::GetMontageInstance(GetAnimInstanceFromActorInfo(), StopMontage))
		{
			theStartPosition += theStopMontageInstance->GetBlendTime() * theStopMontageInstance->GetWeight();
		}
	}

	// Play the skill params
	UAnimMontage* thePlayMontage = nullptr;
	const bool bIsPlaying = PlaySkillMontage(theSkillParamsGroup, thePlayMontage, thePlayRate, theStartPosition);
	if (bIsPlaying == false)
	{	
		// Clear invalid play state.
		TrajectoryGaitAxis.Reset();
		TrajectoryLocomotionStatus.Reset();
		return false;
	}

	// Response montage blend out now...so should clear it.
	TrajectoryGaitAxis.Reset();
	TrajectoryLocomotionStatus.Reset();

	// If is playing the params montage, we should cache the params data
	PivotMontage = thePlayMontage;
	ResponsePivotTime = FPlatformTime::Seconds();
	SetLocomotionStatus(theLocomotionParams->LocomotionStatus);

	// Always cache the current movement direction, it will be used for the comparison of the next frame
    //LastMovementDirectionAsPivot = theMovementComp->GetMovementDirection();

	// Handle movement component data
	if (UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo())
	{
		// Reset the last movement direction
		theMovementComp->ResetMovementDirection();

		// If the params has animated rotation, we should lock rotation input
		if (theLocomotionParams->HasRotation())
		{
			bLockRotationInput = true;
			theMovementComp->SetIgnoreRotationInput(true);
		}
	}

	// Return the result
	return bIsPlaying;
}
#pragma endregion


#pragma region Stop
bool UVMM_GameplayAbility_Locomotion::ResponseMovementStop(const FIntPoint& InGatAxisRange, const ECardinalDirection& InDirection)
{
	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo();
	if (theAnimInstance == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementStop, Invalid animation instance."));
		return false;
	}

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementStop, Invalid locomotion data."));
		return false;
	}

	// Calculate the foot index
	int32 theFootIndex = 1;

	// Calculate the active locomotion montage foot position curve value
#if ENGINE_MAJOR_VERSION > 4
	FVector2f theLastRatePositionData;
#else
	FVector2D theLastRatePositionData;
#endif // ENGINE_MAJOR_VERSION > 4

	UAnimMontage* theLastMontage = GetLocomotionMontage();
	const bool bHasPlayingLastMontage = UVMM_MontageLibrary::GetMontagePlayRatePosition(theAnimInstance, theLastMontage, theLastRatePositionData.X, theLastRatePositionData.Y, false);
	if (bHasPlayingLastMontage)
	{
		// Search the curve data
		bool bHasCurveData = false;
		const float theCurveValue = UVMM_CurveLibrary::GetCurveValueFromMontage(theLastMontage, LocomotionData->GetFootPositionCurveName(), theLastRatePositionData.Y, bHasCurveData);

		// If has the curve data, we should convert to foot position
		if (bHasCurveData)
		{
			theFootIndex = FMath::TruncToInt(theCurveValue);
		}
	}

	// Check the params is valid
	const FLocomotionParams* theLocomotionParams = LocomotionData->GetStopParams(InGatAxisRange, theFootIndex, InDirection);
	if (theLocomotionParams == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementStop, Invalid locomotion params, Gait: %s, Foot: %d, Direction: %s")
			, *InGatAxisRange.ToString(), theFootIndex, *UVMM_Library::GetCardinalDirectionString(InDirection));
		return false;
	}

	// Define the skill params group data
	const FVirtualSkillParamsGroup theSkillParamsGroup(LocomotionData->GetDataIndex(), theLocomotionParams->Index);

	// Calculate the start position
	float thePlayRate = theLocomotionParams->PlayRate;
	float theStartPosition = theLocomotionParams->StartPosition;

	// Calculate meta start position
	if (bHasPlayingLastMontage)
	{
		switch (LocomotionStatus)
		{
		case ELocomotionStatus::START:
		case ELocomotionStatus::PIVOT:
			if (theLastRatePositionData.Y < 0.4f) // Should calculate dynamic speed
			{
				if (theLocomotionParams->MetaStartPosition.Num() > 0)
				{
					theStartPosition = FMath::Max(theStartPosition, theLocomotionParams->MetaStartPosition[0]);
				}
			}
			break;
		}
	}

	// Always clear same status bind event
	if (true/*LocomotionStatus == ELocomotionStatus::STOP*/)
	{
		UVMM_MontageLibrary::ClearMontageBindEvent(GetAnimInstanceFromActorInfo(), StopMontage);
	}

	// Play the skill params
	UAnimMontage* thePlayMontage = nullptr;
	const bool bIsPlaying = PlaySkillMontage(theSkillParamsGroup, thePlayMontage, thePlayRate, theStartPosition);
	if (bIsPlaying == false)
	{
		return false;
	}

	// If is playing the params montage, we should cache the params data
	StopMontage = thePlayMontage;
	StopMontageDirection = theLocomotionParams->CardinalDirection;
	SetLocomotionStatus(theLocomotionParams->LocomotionStatus);

	// Bind wait early blend out event, We can quickly respond to input through this tag event
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent)
	{
		EarlyStopBlendOutEventHandle = theAbilityComponent->AddGameplayEventTagContainerDelegate(EarlyStopBlendOutTags, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnEarlyStopBlendOutEvent));
	}

	// Return the result
	return bIsPlaying;
}

void UVMM_GameplayAbility_Locomotion::OnEarlyStopBlendOutEvent(FGameplayTag InEventTag, const FGameplayEventData* InPayload)
{
	// Remove the event bind
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent)
	{
		theAbilityComponent->RemoveGameplayEventTagContainerDelegate(EarlyStopBlendOutTags, EarlyStopBlendOutEventHandle);
	}

	// Response movement direction
	if (UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo())
	{
		if (theMovementComp->HasMovementInput())
		{
			// Check the animation instance is valid
			UAnimInstance* theAnimInstance = GetAnimInstanceFromActorInfo();
			if (theAnimInstance == nullptr)
			{
				return;
			}

			// Stop the montage
			UVMM_MontageLibrary::MontageStop(theAnimInstance, StopMontage);
		}
	}
}
#pragma endregion


#pragma region Jump
bool UVMM_GameplayAbility_Locomotion::ResponseMovementJump(UVMM_CharacterMovementComponent* InMovementComp, const ELocomotionStatus InStatus)
{
	check(InMovementComp);

	// Check the locomotion data is valid
	if (LocomotionData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_GameplayAbility_Locomotion::ResponseMovementJump, Invalid locomotion data."));
		return false;
	}

	// Get current movement data
	const FIntPoint theGaitAxisRange = InMovementComp->HasMovementInput() ? FIntPoint(0) : FIntPoint(INDEX_NONE);
	const ECardinalDirection theMoveDirection = InMovementComp->GetAdjustMovementCardinalDirection();

	// Check the jump start params is valid
	const FLocomotionParams* theJumpParams = LocomotionData->GetJumpParams(InStatus, theGaitAxisRange, int32(theMoveDirection));
	if (theJumpParams == nullptr)
	{
		return false;
	}

	// Define the skill params group data
	const FVirtualSkillParamsGroup theSkillParamsGroup(LocomotionData->GetDataIndex(), theJumpParams->Index);

	// Calculate the start position
	float thePlayRate = theJumpParams->PlayRate;
	float theStartPosition = theJumpParams->StartPosition;

	// Make a dynamic montage
	UAnimMontage* thePlayMontage = nullptr;

	// Try lock the movement input
	if (InStatus == ELocomotionStatus::JUMPSTART)
	{
		if (ACharacter* theChracter = InMovementComp->GetCharacterOwner())
		{
			if (AController* theOwnerController = theChracter->GetController())
			{
				theOwnerController->SetIgnoreMoveInput(true);
			}
		}
	}

	// Cache current velocity
	const FVector theStartVelocity = InMovementComp->Velocity;

	// Play the skill params
	const bool bIsPlaying = PlaySkillMontage(theSkillParamsGroup, thePlayMontage, thePlayRate, theStartPosition);
	if (bIsPlaying == false)
	{
		return false;
	}

	// If is playing the params montage, we should cache the params data
	JumpMontage = thePlayMontage;
	SetLocomotionStatus(theJumpParams->LocomotionStatus);

	// Handle jump status event
	switch (LocomotionStatus)
	{
	case ELocomotionStatus::JUMPSTART:
		FallingStartLocation = InMovementComp->GetActorFeetLocation();

		// Choose animation or procedural simulate mode
		if (thePlayMontage->HasRootMotion())
		{
			// Set fly or custom movement mode
			InMovementComp->SetMovementMode(MOVE_Flying);

			// Transfer last simulate velocity
			InMovementComp->Velocity.X = theStartVelocity.X;
			InMovementComp->Velocity.Y = theStartVelocity.Y;
		}
		else if (ACharacter* theCharacter = InMovementComp->GetCharacterOwner())
		{
			InMovementComp->bNotifyApex = true;
			theCharacter->Jump();
		}

		// Wait animation notify callback
		{
			UAbilityTask_WaitGameplayEvent* theWaitEvent = UAbilityTask::NewAbilityTask<UAbilityTask_WaitGameplayEvent>(this);
			theWaitEvent->Tag = JumpApexTag;
			theWaitEvent->SetExternalTarget(nullptr);
			theWaitEvent->OnlyTriggerOnce = true;
			theWaitEvent->OnlyMatchExact = true;
			theWaitEvent->EventReceived.AddUniqueDynamic(this, &ThisClass::OnJumpApex);
			theWaitEvent->ReadyForActivation();
		}
		break;

	case ELocomotionStatus::JUMPFALLING:
		FallingMontage = thePlayMontage;
		FallingStartLocation = InMovementComp->GetActorFeetLocation();
		break;

	case ELocomotionStatus::JUMPLAND:
		break;

	case ELocomotionStatus::JUMPMOBILELAND:
		break;
	}

	// Return the result
	return bIsPlaying;
}

void UVMM_GameplayAbility_Locomotion::OnJumpApex(FGameplayEventData InPayload)
{
	// Check the movement component is valid
	UVMM_CharacterMovementComponent* theMovementComp = GetVirtualCharacterMovementComponentFromActorInfo();
	if (theMovementComp == nullptr)
	{
		return;
	}
	
	// Try set falling movement mode
	theMovementComp->SetMovementMode(MOVE_Falling);
}
#pragma endregion


#pragma region Foot Position
int32 UVMM_GameplayAbility_Locomotion::GetFootPosition()
{
	// Calculate the foot index
	int32 theFootIndex = 1;

	// Check the animation instance is valid
	UVMM_AnimInstance* theAnimInstance = Cast<UVMM_AnimInstance>(GetAnimInstanceFromActorInfo());
	if (theAnimInstance == nullptr)
	{
		return theFootIndex;
	}

	// Calculate the active locomotion montage foot position curve value
#if ENGINE_MAJOR_VERSION > 4
	FVector2f theLastRatePositionData;
#else
	FVector2D theLastRatePositionData;
#endif // ENGINE_MAJOR_VERSION > 4

	UAnimMontage* theLastMontage = GetLocomotionMontage();
	const bool bHasPlayingLastMontage = UVMM_MontageLibrary::GetMontagePlayRatePosition(theAnimInstance, theLastMontage, theLastRatePositionData.X, theLastRatePositionData.Y, false);
	if (bHasPlayingLastMontage)
	{
		// Search the curve data
		bool bHasCurveData = false;
		const float theCurveValue = UVMM_CurveLibrary::GetCurveValueFromMontage(theLastMontage, LocomotionData->GetFootPositionCurveName(), theLastRatePositionData.Y, bHasCurveData);

		// If has the curve data, we should convert to foot position
		if (bHasCurveData)
		{
			theFootIndex = FMath::TruncToInt(theCurveValue);
		}
	}

	// Return the result
	return theFootIndex;
}
#pragma endregion