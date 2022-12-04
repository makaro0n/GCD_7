// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "GameFramework/VMM_AnimInstance.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Librarys/VMM_MontageLibrary.h"
#include "GameFramework/VMM_Character.h"
#include "GameFramework/VMM_CharacterMovementComponent.h"
#include "DataAssets/VMM_LocomotionData.h"
#include "DataRegistrySubsystem.h"


UVMM_AnimInstance::UVMM_AnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsPreviousLayer(false)
	, AnimLayerWeight(1.f)

	, bEnableAimOffset(true)
	, AimOffsetWeight(1.f)
	, DesiredAimOffsetWeight(1.f)
	, AimOffsetBlendInput(FVector2D(0.f, 0.f))
	, AimOffsetInterpSpeed(20.f)
	, AimOffsetWeightInterpSpeed(5.f)

	, RotationYawOffsetRange(FVector2D(-45.f, 45.f))
	, RotationOffset(ForceInitToZero)
	, RotationOffsetAlpha(0.f)
{}

#pragma region Owner
void UVMM_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache the character reference
	AVMM_Character* theCharacterOwner = Cast<AVMM_Character>(GetOwningActor());
	if (theCharacterOwner)
	{
		CharacterOwner = theCharacterOwner;
		CharacterMovementComp = theCharacterOwner->CharacterMovementComponent;
		theCharacterOwner->CharacterStanceDelegate.AddUniqueDynamic(this, &ThisClass::OnStanceChanged);
	}

#if 1 // In GAS_Locomotion
	if (theCharacterOwner && theCharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		return;
	}

	// Initialize animation dataset
	SetAnimRegistryId(InitAnimRegistryId);
#endif 
}

void UVMM_AnimInstance::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();

	// Cache the character reference
	AVMM_Character* theCharacterOwner = Cast<AVMM_Character>(GetOwningActor());
	if (theCharacterOwner)
	{
		theCharacterOwner->CharacterStanceDelegate.RemoveDynamic(this, &ThisClass::OnStanceChanged);
	}

	// Clear all delegates
	LocomotionDataDelegate.Clear();
}

void UVMM_AnimInstance::NativeUpdateAnimation(float InDeltaSeconds)
{
	Super::NativeUpdateAnimation(InDeltaSeconds);

	// Check the owner is valid
	if (!CharacterOwner || !CharacterMovementComp)
	{
		return;
	}

	// Calculate view rotation
	CalculateViewRotation(InDeltaSeconds);

	// Update thread safe data
	CalculateRotationOffset(InDeltaSeconds);
}

void UVMM_AnimInstance::Montage_Advance(float InDeltaSeconds)
{
	Super::Montage_Advance(InDeltaSeconds);
}
#pragma endregion


#pragma region Eye
FVector UVMM_AnimInstance::GetEyeLocation() const
{
	check(GetOwningComponent());
	return GetOwningComponent() ? GetOwningComponent()->GetSocketLocation(GetEyeSocketName()) : FVector::ZeroVector;
}
#pragma endregion


#pragma region Data
bool UVMM_AnimInstance::InitializeAnimRegistryId()
{
	return SetAnimRegistryId(InitAnimRegistryId);
}

bool UVMM_AnimInstance::SetAnimRegistryId(const FDataRegistryId& InRegistryId)
{
#if 0
	// Check the registry id is valid
	if (AnimRegistryId == InRegistryId)
	{
		return;
	}
#endif

	// Cache the new animation registry id
	AnimRegistryId = InRegistryId;

	// Check the data registry system is valid
	UDataRegistrySubsystem* theDataRegistrySubsystem = UDataRegistrySubsystem::Get();
	if (theDataRegistrySubsystem == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_AnimInstance::SetAnimRegistryId, Invalid data registry system: %s"), *InRegistryId.ToString());
		return false;
	}

	// Check the animation style dataset is valid
	const FMotionAnimDataset* theAnimDataset = theDataRegistrySubsystem->GetCachedItem<FMotionAnimDataset>(InRegistryId);
	if (theAnimDataset == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_AnimInstance::SetAnimRegistryId, Invalid animation data: %s"), *InRegistryId.ToString());
		return false;
	}

	// Cache the animation dataset
	AnimDataset = *theAnimDataset;

	// Update locomotion data
	return UpdateLocomotionData();
}

bool UVMM_AnimInstance::UpdateLocomotionData()
{
	// Check the movement or ability component is valid
	if (!CharacterMovementComp)
	{
		return false;
	}

	// Get the stance gameplay tag
	const FGameplayTag& theStanceTag = CharacterMovementComp->IsCrouching() ? CrouchTag : StandTag;

	// Get the rotation mode
	const ERotationMode theRotationMode = CharacterMovementComp->GetRotationMode();

	// Define the aim state
	const bool bIsAiming = theRotationMode == ERotationMode::UnEasy || theRotationMode == ERotationMode::Combat;

	// Check the stance locomotion data is valid
	UVMM_LocomotionData* const* theLocomotionData = bIsAiming ? AnimDataset.AimLocomotionDataset.Find(theStanceTag) : AnimDataset.RlxLocomotionDataset.Find(theStanceTag);
	if (theLocomotionData == nullptr)
	{
		return false;
	}

	// Check the new data is valid
	if (*theLocomotionData == GetLocomotionData())
	{
		return false;
	}

	// Refresh the locomotion data
	SetLocomotionData(*theLocomotionData);

	// Return the result
	return true;
}

void UVMM_AnimInstance::SetLocomotionData(const UVMM_LocomotionData* InData)
{
	// Inverse the layer state
	bIsPreviousLayer = !bIsPreviousLayer;
	LastAnimLayerWeight = AnimLayerWeight;
	RevertAfterLayerWeight();

	// Cache the data asset
	if (bIsPreviousLayer)
	{
		PreviousLocomotionData = InData;
	}
	else
	{
		FutureLocomotionData = InData;
	}

	// Call delegate
	if (LocomotionDataDelegate.IsBound())
	{
		LocomotionDataDelegate.Broadcast(InData);
	}
}

const FIntPoint UVMM_AnimInstance::GetGaitAxisRange() const
{
	return CharacterMovementComp ? CharacterMovementComp->GetGaitAxisRange() : FIntPoint(0);
}

UAnimSequence* UVMM_AnimInstance::GetFutureBasePose() const
{
	return FutureLocomotionData == NULL ? nullptr : FutureLocomotionData->BasePose;
}

UBlendSpace* UVMM_AnimInstance::GetFutureAimOffset() const
{
	return FutureLocomotionData == NULL ? nullptr : FutureLocomotionData->GetAimOffsetAsset();
}

UBlendSpace* UVMM_AnimInstance::GetFutureRotationOffset() const
{
	return FutureLocomotionData == NULL ? nullptr : FutureLocomotionData->GetRotationOffsetAsset(GetGaitAxisRange());
}

UAnimSequence* UVMM_AnimInstance::GetPreviousBasePose() const
{
	return PreviousLocomotionData == NULL ? nullptr : PreviousLocomotionData->BasePose;
}

UBlendSpace* UVMM_AnimInstance::GetPreviousAimOffset() const
{
	return PreviousLocomotionData == NULL ? nullptr : PreviousLocomotionData->GetAimOffsetAsset();
}

UBlendSpace* UVMM_AnimInstance::GetPreviousRotationOffset() const
{
	return PreviousLocomotionData == NULL ? nullptr : PreviousLocomotionData->GetRotationOffsetAsset(GetGaitAxisRange());
}
#pragma endregion


#pragma region State
void UVMM_AnimInstance::OnStanceChanged(ACharacter* InCharacter)
{
	UpdateLocomotionData();
}
#pragma endregion


#pragma region Pose Layer
void UVMM_AnimInstance::SetFirstPersonMode(bool InState)
{
	bIsFirstPerson = InState;
	K2_SetFirstPersonMode(InState);
}
#pragma endregion


#pragma region Animation Layer
void UVMM_AnimInstance::RevertBeforeLayerWeight()
{
	AnimLayerWeight = LastAnimLayerWeight;
}

void UVMM_AnimInstance::RevertAfterLayerWeight()
{
	AnimLayerWeight = bIsPreviousLayer ? 1.f : 0.f;
}
#pragma endregion


#pragma region Lower Body Layer
void UVMM_AnimInstance::SetLowerBodyMontage(UAnimMontage* InMontage)
{
	LowerBodyMontage = InMontage;
}

void UVMM_AnimInstance::SetLowerBodyLockMontage(UAnimMontage* InMontage)
{
	LowerBodyLockMontage = InMontage;
}

bool UVMM_AnimInstance::IsLowerBodyLayerMontage(UAnimMontage* InMontage) const
{
	check(InMontage);

	// Each every slot animation track
	for (FSlotAnimationTrack& theSlotTrack : InMontage->SlotAnimTracks)
	{
		// Check the desired slot name is valid
		if (LowerBodySlotData.SlotsName.Contains(theSlotTrack.SlotName))
		{
			return true;
		}
	}

	// Failed
	return false;
}

bool UVMM_AnimInstance::IsLowerBodyLayerMontageSlot(const TArray<FName>& InSlotsName) const
{
	// Each every slot animation track
	for (const FName& theSlotName : InSlotsName)
	{
		// Check the desired slot name is valid
		if (LowerBodySlotData.SlotsName.Contains(theSlotName))
		{
			return true;
		}
	}

	// Failed
	return false;
}

bool UVMM_AnimInstance::IsPlayingLowerBodyMontage(const bool InActiveMontage)
{
	return UVMM_MontageLibrary::GetMontageIsPlaying(this, LowerBodyMontage);
}
#pragma endregion


#pragma region Upper Body Layer
void UVMM_AnimInstance::SetUpperBodyMontage(UAnimMontage* InMontage)
{
	UpperBodyMontage = InMontage;
}

void UVMM_AnimInstance::SetUpperIgnoreBodyMontage(UAnimMontage* InMontage)
{
	UpperIgnoreBodyMontage = InMontage;
}

bool UVMM_AnimInstance::IsUpperBodyLayerMontage(UAnimMontage* InMontage) const
{
	check(InMontage);

	// Each every slot animation track
	for (FSlotAnimationTrack& theSlotTrack : InMontage->SlotAnimTracks)
	{
		// Check the desired slot name is valid
		if (UpperBodySlotData.SlotsName.Contains(theSlotTrack.SlotName))
		{
			return true;
		}
	}

	// Failed
	return false;
}

bool UVMM_AnimInstance::IsUpperBodyLayerMontageSlot(const TArray<FName>& InSlotsName) const
{
	// Each every slot animation track
	for (const FName& theSlotName : InSlotsName)
	{
		// Check the desired slot name is valid
		if (UpperBodySlotData.SlotsName.Contains(theSlotName))
		{
			return true;
		}
	}

	// Failed
	return false;
}

bool UVMM_AnimInstance::IsPlayingUpperBodyMontage(const bool InActiveMontage)
{
	return UVMM_MontageLibrary::GetMontageIsPlaying(this, UpperBodyMontage);
}
#pragma endregion


#pragma region Whole Body Layer
void UVMM_AnimInstance::SetWholeBodyMontage(UAnimMontage* InMontage)
{
	WholeBodyMontage = InMontage;
}

bool UVMM_AnimInstance::IsWholeBodyLayerMontage(UAnimMontage* InMontage) const
{
	check(InMontage);

	// Each every slot animation track
	for (FSlotAnimationTrack& theSlotTrack : InMontage->SlotAnimTracks)
	{
		// Check the desired slot name is valid
		if (WholeBodySlotData.SlotsName.Contains(theSlotTrack.SlotName))
		{
			return true;
		}
	}

	// Failed
	return false;
}

bool UVMM_AnimInstance::IsWholeBodyLayerMontageSlot(const TArray<FName>& InSlotsName) const
{
	// Each every slot animation track
	for (const FName& theSlotName : InSlotsName)
	{
		// Check the desired slot name is valid
		if (WholeBodySlotData.SlotsName.Contains(theSlotName))
		{
			return true;
		}
	}

	// Failed
	return false;
}

bool UVMM_AnimInstance::IsPlayingWholeBodyMontage(const bool InActiveMontage)
{
	return UVMM_MontageLibrary::GetMontageIsPlaying(this, WholeBodyMontage);
}
#pragma endregion


#pragma region View
void UVMM_AnimInstance::CalculateViewRotation(const float& InDeltaSeconds)
{
	if (AimOffsetWeight != DesiredAimOffsetWeight)
	{
		AimOffsetWeight = FMath::FInterpTo(AimOffsetWeight, DesiredAimOffsetWeight, InDeltaSeconds, AimOffsetWeightInterpSpeed);
	}

	// If it is a free view, we should clear the aiming offset
	if (!FAnimWeight::IsFullWeight(DesiredAimOffsetWeight))
	{
		// Make to zero
		if (!AimOffsetBlendInput.IsZero())
		{
			AimOffsetBlendInput *= AimOffsetWeight;
		}
		return;
	}

	// Get character view rotation
	FRotator theEyeRotation = FRotator::ZeroRotator;
	if (CharacterOwner->IsLookingRotationMode())
	{
		theEyeRotation = (CharacterOwner->GetLookingRotation() - CharacterOwner->GetActorRotation());
		theEyeRotation.Normalize();
	}

	// Interp
	AimOffsetBlendInput = FMath::Vector2DInterpTo(AimOffsetBlendInput, FVector2D(theEyeRotation.Yaw, theEyeRotation.Pitch), InDeltaSeconds, AimOffsetInterpSpeed);
}
#pragma endregion


#pragma region Rotation
float UVMM_AnimInstance::GetRotationOffsetAlpha()
{
	// Check the movement is valid
	check(CharacterMovementComp);

	// Define the base alpha value
	float theAlpha = 0.f;

	// Check the rotation input state
	if (CharacterMovementComp->IsUpdateRotationIgnored())
	{
		return theAlpha;
	}

	// Interp max alpha
	const float theSpeed = CharacterMovementComp->Velocity.Size();
	if (CharacterMovementComp->HasMovementInput() || theSpeed > 0.f)
	{
		theAlpha = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 100.f), FVector2D(0.f, 1.f), theSpeed);
	}

	// Calculate the animation source weight
	if (GetLocomotionData())
	{
		const float theAnimAlpha = GetLocomotionData()->GetRotationOffsetAlpha(GetGaitAxisRange());
		theAlpha *= theAnimAlpha;
	}

	// Return the result
	return theAlpha;
}

void UVMM_AnimInstance::CalculateRotationOffset(const float& InDeltaSeconds)
{
	// Calculate the rotation offset alpha
	const float& theDesiredAlpha = GetRotationOffsetAlpha();

	// Interp the alpha
	if (theDesiredAlpha >= RotationOffsetAlpha)
	{
		RotationOffsetAlpha = FMath::FInterpTo(RotationOffsetAlpha, theDesiredAlpha, InDeltaSeconds, 5.f);
	}
	else
	{
		RotationOffsetAlpha = FMath::FInterpConstantTo(RotationOffsetAlpha, theDesiredAlpha, InDeltaSeconds, 5.f);
	}

#if 1
	// Check the alpha is valid
	if (!FAnimWeight::IsRelevant(RotationOffsetAlpha))
	{
		RotationOffset = FVector2D::ZeroVector;
		return;
	}
#endif

	// Define the desired rotation offset
	FVector2D theDesiredRotationOffset = RotationOffset;

	// Get character view rotation
	FRotator theCharacterEyeRotation = (CharacterOwner->GetLookingRotation() - CharacterOwner->GetActorRotation());
	theCharacterEyeRotation.Normalize();

	// Get the input rotation
	const FRotator& theInputRotation = CharacterMovementComp->GetAdjustMovementRotation();

	// Calculate the input rotation angle
	theCharacterEyeRotation = (FQuat(theInputRotation) * FQuat(theCharacterEyeRotation)).Rotator();

	// Calculate the yaw angle offset in two frame
	const float theYawDeltaSinceLastUpdate = CharacterOwner->GetActorRotation().Yaw - LastCharacterRotation.Yaw;
	const float theAbstheYawDeltaSinceLastUpdate = FMath::Abs(theYawDeltaSinceLastUpdate);
	LastCharacterRotation = CharacterOwner->GetActorRotation();

	// Check the circle angle weight
	if (theAbstheYawDeltaSinceLastUpdate <= KINDA_SMALL_NUMBER)
	{
		theDesiredRotationOffset.X = 0.f;
	}
	else if (theAbstheYawDeltaSinceLastUpdate > FMath::Abs(theCharacterEyeRotation.Yaw))
	{
		theDesiredRotationOffset.X = theYawDeltaSinceLastUpdate * 2.5f;
	}
	else if (theAbstheYawDeltaSinceLastUpdate <= 0.5f)
	{
		theDesiredRotationOffset.X = theYawDeltaSinceLastUpdate;
	}
	else
	{
		theDesiredRotationOffset.X = theCharacterEyeRotation.Yaw;
	}

	// Clamp the rotation offset
	theDesiredRotationOffset.X = FMath::Clamp(theDesiredRotationOffset.X, RotationYawOffsetRange.X, RotationYawOffsetRange.Y);

	// Get the movement cardinal direction type
	const ECardinalDirection& theCardinalDirection = CharacterMovementComp->GetAdjustMovementCardinalDirection();

	// Calculate the rotation direction
	switch (theCardinalDirection)
	{
	case ECardinalDirection::Left:
		theDesiredRotationOffset.X *= (theDesiredRotationOffset.X > 0.f ? -0.5f : -0.25f);
		break;

	case ECardinalDirection::Right:
		theDesiredRotationOffset.X *= (theDesiredRotationOffset.X >= 0.f ? 0.1f : 0.5f);
		break;

	case ECardinalDirection::Backward:
		theDesiredRotationOffset.X *= -1.f;
		break;
	}

	// Define the abs value
	const float pA = FMath::Abs(RotationOffset.X);
	const float pB = FMath::Abs(theDesiredRotationOffset.X);

	// Clamp the circle angle
	theDesiredRotationOffset.X = pB <= 0.1f ? 0.f : theDesiredRotationOffset.X;

	// Smooth very low value
	const float theTolerance = 10.f;
	if (false/*pB < pA && ((pA < theTolerance && pB < theTolerance) || (pA > theTolerance && pB < theTolerance))*/)
	{
		RotationOffset.X = FMath::FInterpTo(RotationOffset.X, theDesiredRotationOffset.X, InDeltaSeconds, 5.f);
	}
	else
	{
		//RotationOffset.X = theDesiredRotationOffset.X;
		const float theInterpSpeed = FMath::Abs(theDesiredRotationOffset.X) >= FMath::Abs(RotationOffset.X) ? 8.f : 2.5f;
		RotationOffset.X = FMath::FInterpTo(RotationOffset.X, theDesiredRotationOffset.X, InDeltaSeconds, theInterpSpeed);
	}

#if 0
	FString theDebugString = FString::Printf(TEXT("AnimInstance, Eye: %f, LastUpdate; %f, Offset: %f")
		, theCharacterEyeRotation.Yaw, theYawDeltaSinceLastUpdate, RotationOffset.X);
	UKismetSystemLibrary::PrintString(this, theDebugString, true, true, FLinearColor::Green, 0.f);
#endif
}
#pragma endregion