// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Librarys/VMM_MontageLibrary.h"
#include "VirtualMotionMatching.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Librarys/VMM_CurveLibrary.h"

FAnimMontageInstance* UVMM_MontageLibrary::GetMontageInstance(UAnimInstance* InAnimInstance, UAnimMontage* InMontage)
{
	// Always check the animation instance is valid
	if (InAnimInstance == nullptr)
	{
		return nullptr;
	}

	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return nullptr;
	}

	// Each every montage instance
	for (FAnimMontageInstance* theMontageInstance : InAnimInstance->MontageInstances)
	{
		// Check the montage instance is valid
		if (theMontageInstance == nullptr)
		{
			return nullptr;
		}

		// Check is equal desired montage
		if (theMontageInstance->Montage == InMontage)
		{
			return theMontageInstance;
		}
	}

	// Failed
	return nullptr;
}

FAnimMontageInstance* UVMM_MontageLibrary::GetActiveInstanceForMontage(UAnimInstance* InAnimInstance, UAnimMontage* InMontage)
{
	// Always check the animation instance is valid
	if (InAnimInstance == nullptr)
	{
		return nullptr;
	}

	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return nullptr;
	}

	// Return desired result
	return InAnimInstance->GetActiveInstanceForMontage(InMontage);
}

void UVMM_MontageLibrary::SetRootMotion(UAnimSequence* InAnimSequence, bool InEnableRootMotion)
{
	if (!InAnimSequence)
	{
		return;
	}

	InAnimSequence->bEnableRootMotion = InEnableRootMotion;
	InAnimSequence->bForceRootLock = !InEnableRootMotion;

#if WITH_EDITOR
	InAnimSequence->PostEditChange();
	InAnimSequence->RefreshCacheData();
	InAnimSequence->MarkPackageDirty();

#if ENGINE_MAJOR_VERSION < 5
	InAnimSequence->RefreshCurveData();
	InAnimSequence->MarkRawDataAsModified();
#endif

#endif
}

bool UVMM_MontageLibrary::GetMontageIsPlaying(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, const bool InActiveMontage)
{
	// Check the animation instance is valid
	if (InAnimInstance == nullptr)
	{
		return false;
	}

	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return false;
	}

	// Return
	if (InActiveMontage)
	{
		const FAnimMontageInstance* theMontageInstance = InAnimInstance->GetActiveInstanceForMontage(InMontage);
		if (theMontageInstance != nullptr)
		{
			return theMontageInstance->IsPlaying();
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		for (int32 InstanceIndex = 0; InstanceIndex < InAnimInstance->MontageInstances.Num(); InstanceIndex++)
		{
			const FAnimMontageInstance* theMontageInstance = InAnimInstance->MontageInstances[InstanceIndex];
			if (theMontageInstance != nullptr && theMontageInstance->Montage == InMontage)
			{
				return theMontageInstance->IsPlaying();
			}
		}
	}

	return false;
}

float UVMM_MontageLibrary::GetMontagePlayPosition(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, const bool InActiveMontage /*= true*/)
{
	// Check the animation instance is valid
	if (InAnimInstance == nullptr)
	{
		return 0.f;
	}

	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return 0.f;
	}

	// Return
	if (InActiveMontage)
	{
		const FAnimMontageInstance* theMontageInstance = InAnimInstance->GetActiveInstanceForMontage(InMontage);
		if (theMontageInstance != nullptr)
		{
			return theMontageInstance->GetPosition();
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		for (int32 InstanceIndex = 0; InstanceIndex < InAnimInstance->MontageInstances.Num(); InstanceIndex++)
		{
			const FAnimMontageInstance* theMontageInstance = InAnimInstance->MontageInstances[InstanceIndex];
			if (theMontageInstance != nullptr && theMontageInstance->Montage == InMontage)
			{
				return theMontageInstance->GetPosition();
			}
		}
	}

	return 0.f;
}

bool UVMM_MontageLibrary::GetMontagePlayRatePosition(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, float& OutPlayRate, float& OutPlayPosition, const bool InActiveMontage /*= true*/)
{
	// Check the desired montage instance is valid
	FAnimMontageInstance* theMontageInstance = InActiveMontage ? GetActiveInstanceForMontage(InAnimInstance, InMontage) : GetMontageInstance(InAnimInstance, InMontage);
	if (theMontageInstance)
	{
		OutPlayRate = theMontageInstance->GetPlayRate();
		OutPlayPosition = theMontageInstance->GetPosition();
		return true;
	}

	// Failed
	OutPlayRate = OutPlayPosition = 0.f;
	return false;
}

bool UVMM_MontageLibrary::GetMontagePlaySection(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, FName& OutSectionName, const bool InActiveMontage /*= true*/)
{
	// Check the desired montage instance is valid
	FAnimMontageInstance* theMontageInstance = InActiveMontage ? GetActiveInstanceForMontage(InAnimInstance, InMontage) : GetMontageInstance(InAnimInstance, InMontage);
	if (theMontageInstance)
	{
		OutSectionName = theMontageInstance->GetCurrentSection();
		return true;
	}

	// INVALID
	return false;
}

bool UVMM_MontageLibrary::GetMontagePlayNextSection(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, FName& OutSectionName, FName& OutNextSectionName, const bool InActiveMontage /*= true*/)
{
	// Check the desired montage instance is valid
	FAnimMontageInstance* theMontageInstance = InActiveMontage ? GetActiveInstanceForMontage(InAnimInstance, InMontage) : GetMontageInstance(InAnimInstance, InMontage);
	if (theMontageInstance)
	{
		OutSectionName = theMontageInstance->GetCurrentSection();
		OutNextSectionName = theMontageInstance->GetNextSection();
		return true;
	}

	// INVALID
	return false;
}

float UVMM_MontageLibrary::GetMontageBlendWeight(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, const bool InActiveMontage /*= true*/)
{
	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return 0.f;
	} 

	// Check the montage instance is valid
	FAnimMontageInstance* theMontageInstance = GetMontageInstance(InAnimInstance, InMontage);
	if (theMontageInstance == nullptr)
	{
		return 0.f;
	}

	// Check the desired weight is valid
	if (theMontageInstance->GetDesiredWeight() == 0.f)
	{
		if (theMontageInstance->IsStopped())
		{
			return theMontageInstance->GetWeight();
		}
		return 0.f;
	}

	// Return the weight
	return theMontageInstance->GetWeight() / theMontageInstance->GetDesiredWeight();
}

bool UVMM_MontageLibrary::HasPlayingMontages(UAnimInstance* InAnimInstance, TArray<UAnimMontage*> InMontages, const bool InActiveMontage, int32& OutCount)
{
	OutCount = 0;
	if (InActiveMontage)
	{
		// If no Montage reference, use first active one found.
		for (int32 InstanceIndex = 0; InstanceIndex < InAnimInstance->MontageInstances.Num(); InstanceIndex++)
		{
			const FAnimMontageInstance* theMontageInstance = InAnimInstance->MontageInstances[InstanceIndex];
			if (theMontageInstance != nullptr && theMontageInstance->IsActive())
			{
				if (InMontages.Contains(theMontageInstance->Montage))
				{
					++OutCount;
				}
			}
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		for (int32 InstanceIndex = 0; InstanceIndex < InAnimInstance->MontageInstances.Num(); InstanceIndex++)
		{
			const FAnimMontageInstance* theMontageInstance = InAnimInstance->MontageInstances[InstanceIndex];
			if (theMontageInstance != nullptr)
			{
				if (InMontages.Contains(theMontageInstance->Montage))
				{
					++OutCount;
				}
			}
		}
	}

	// Return the result
	return OutCount > 0;
}

void UVMM_MontageLibrary::PushDisableRootMotion(UAnimInstance* InAnimInstance, UAnimMontage* InMontage)
{
	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return;
	}

	// Check the montage instance is valid
	FAnimMontageInstance* theMontageInstance = GetMontageInstance(InAnimInstance, InMontage);
	if (theMontageInstance == nullptr)
	{
		return;
	}

	// Disable
	theMontageInstance->PushDisableRootMotion();
	//InAnimInstance->ClearMontageInstanceReferences(*theMontageInstance);
}

bool UVMM_MontageLibrary::MontageStop(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, float InBlendOutTime /*= -1.f*/, bool bInterrupt /*= true*/)
{
	// Always check the animation instance is valid
	if (InAnimInstance == nullptr)
	{
		return false;
	}

	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return false;
	}

	// Calculate the blend out time
	InBlendOutTime = InBlendOutTime >= 0.f ? InBlendOutTime : InMontage->BlendOut.GetBlendTime();

	// Check the desired montage instance is valid
	FAnimMontageInstance* theMontageInstance = GetActiveInstanceForMontage(InAnimInstance, InMontage);
	if (theMontageInstance == nullptr)
	{
		return false;
	}

	// Success
	theMontageInstance->Stop(FAlphaBlend(InMontage->BlendOut, InBlendOutTime), bInterrupt);
	return true;
}

void UVMM_MontageLibrary::SetMontageBlendTime(UAnimMontage* InMontage, const bool InBlendOut, const float InBlendTime)
{
	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return;
	}

	// Initialize
	InBlendOut ? InMontage->BlendOut.SetBlendTime(InBlendTime) : InMontage->BlendIn.SetBlendTime(InBlendTime);
}

void UVMM_MontageLibrary::MatchingMontageBlendRemainingTime(UAnimInstance* InAnimInstance, UAnimMontage* InLastMontage, UAnimMontage* InNextMontage)
{
	// Check the next montage is valid
	if (InNextMontage == nullptr)
	{
		return;
	}

	// Check the last montage instance is valid
	FAnimMontageInstance* theLastMontageInstance = GetMontageInstance(InAnimInstance, InLastMontage);
	if (theLastMontageInstance == nullptr)
	{
		return;
	}

	// Calculate the remaining time
	const float theLastBlendTimeRemaining = theLastMontageInstance->GetBlendTime() * (theLastMontageInstance->GetWeight() - theLastMontageInstance->GetDesiredWeight());

	// Matching blend in data
	InNextMontage->BlendIn.SetBlendTime(theLastBlendTimeRemaining);
	InNextMontage->BlendIn.SetBlendOption(InLastMontage->BlendOut.GetBlendOption());
}

UAnimMontage* UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage(UObject* InWorldContextObject, const FAnimSkillParams* InSkillParams)
{
	// If the montage asset is valid, we always return the asset
	if (InSkillParams->Montage != NULL)
	{
		return InSkillParams->Montage;
	}

	// Check the param asset is valid
	UAnimSequenceBase* theAnimAsset = InSkillParams->Asset;
	if (theAnimAsset == nullptr || !theAnimAsset->GetSkeleton())
	{
		UE_LOG(LogVirtualMotionMatching, VeryVerbose, TEXT("UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage, Invalid asset."));
		return nullptr;
	}
	USkeleton* theAssetSkeleton = theAnimAsset->GetSkeleton();

	// If the asset is montage, we always return the result
	if (theAnimAsset->StaticClass() == UAnimMontage::StaticClass())
	{
		UE_LOG(LogVirtualMotionMatching, VeryVerbose, TEXT("UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage, Always montage asset."));
		return Cast<UAnimMontage>(theAnimAsset);
	}

	// Check the montage slots name is valid
	if (InSkillParams->MontageSlotsName.Num() == 0)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage, Invalid montage slots data."));
		return nullptr;
	}

	// Check the montage slots name is valid
	if (!theAnimAsset->CanBeUsedInComposition())
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage, This animation isn't supported to play as montage."));
		return nullptr;
	}

	// Create unique montage reference
	FName theMontageName = MakeUniqueObjectName(InWorldContextObject, UAnimMontage::StaticClass(), FName(*theAnimAsset->GetName()));
	UAnimMontage* theNewMontage = NewObject<UAnimMontage>(InWorldContextObject, theMontageName);

	// Initialize the skeleton
	theNewMontage->SetSkeleton(theAssetSkeleton);

	// Initialize the animation track data
	FSlotAnimationTrack& NewTrack = theNewMontage->SlotAnimTracks[0];
	NewTrack.SlotName = InSkillParams->MontageSlotsName[0];
	FAnimSegment NewSegment;
	NewSegment.AnimReference = theAnimAsset;
	NewSegment.AnimStartTime = 0.f;
	NewSegment.AnimEndTime = UVMM_CurveLibrary::GetSequenceLength(theAnimAsset);
	NewSegment.AnimPlayRate = 1.f;
	NewSegment.StartPos = 0.f;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	theNewMontage->SequenceLength = NewSegment.GetLength();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	NewTrack.AnimTrack.AnimSegments.Add(NewSegment);

	// Initialize the animation section data
	FCompositeSection NewSection;
	NewSection.SectionName = TEXT("Default");
	NewSection.LinkSequence(theAnimAsset, UVMM_CurveLibrary::GetSequenceLength(theAnimAsset));
	NewSection.SetTime(0.0f);
	if (InSkillParams->bLooping)
	{
		NewSection.NextSectionName = NewSection.SectionName;
	}

	// add new section
	theNewMontage->CompositeSections.Add(NewSection);
	theNewMontage->BlendIn = InSkillParams->BlendIn;
	theNewMontage->BlendOut = InSkillParams->BlendOut;
	theNewMontage->BlendOutTriggerTime = InSkillParams->BlendOutTriggerTime;

	// Create all montage slot track
	for (int32 SlotIndex = 1; SlotIndex < InSkillParams->MontageSlotsName.Num(); SlotIndex++)
	{
		int32 NewSlot = theNewMontage->SlotAnimTracks.AddDefaulted(1);
		theNewMontage->SlotAnimTracks[NewSlot] = NewTrack;
	}

	// Return the result
	return theNewMontage;
}

void UVMM_MontageLibrary::DebugMontageInstances(UAnimInstance* InAnimInstance)
{
	// Always check the animation instance is valid
	if (InAnimInstance == nullptr)
	{
		return;
	}

	UKismetSystemLibrary::PrintString(InAnimInstance, TEXT(""));
	FString thePrintString = FString::Printf(TEXT("UVMM_MontageLibrary::DebugMontageInstances: %d"), GFrameCounter);
	UKismetSystemLibrary::PrintString(InAnimInstance, thePrintString);

	// Each every montage instance
	for (FAnimMontageInstance* theMontageInstance : InAnimInstance->MontageInstances)
	{
		if (theMontageInstance && theMontageInstance->Montage)
		{
			FString theNewPrintString = FString::Printf(TEXT("Frame: %d, Montage:%s, Pos: %f, Rate: %f, Weight: %f")
				, GFrameCounter, *theMontageInstance->Montage->GetName(), theMontageInstance->GetPosition()
				, theMontageInstance->GetPlayRate(), theMontageInstance->GetWeight());
			UKismetSystemLibrary::PrintString(InAnimInstance, theNewPrintString);
		}
	}
	UKismetSystemLibrary::PrintString(InAnimInstance, TEXT(""));
}

#pragma region Montage Event
void UVMM_MontageLibrary::ClearMontageBindEvent(UAnimInstance* InAnimInstance, UAnimMontage* InMontage)
{
	// Check the montage instance is valid
	FAnimMontageInstance* theMontageInstance = GetMontageInstance(InAnimInstance, InMontage);
	if (theMontageInstance == nullptr)
	{
		return;
	}

	theMontageInstance->OnMontageBlendingOutStarted.Unbind();
	theMontageInstance->OnMontageBlendingOutStarted = nullptr;
	theMontageInstance->OnMontageEnded.Unbind();
	theMontageInstance->OnMontageEnded = nullptr;
}

void UVMM_MontageLibrary::ClearMontageBindOutEvent(UAnimInstance* InAnimInstance, UAnimMontage* InMontage)
{
	// Check the montage instance is valid
	FAnimMontageInstance* theMontageInstance = GetMontageInstance(InAnimInstance, InMontage);
	if (theMontageInstance == nullptr)
	{
		return;
	}

	theMontageInstance->OnMontageBlendingOutStarted.Unbind();
	theMontageInstance->OnMontageBlendingOutStarted = nullptr;
}

void UVMM_MontageLibrary::ClearMontageBindEndedEvent(UAnimInstance* InAnimInstance, UAnimMontage* InMontage)
{
	// Check the montage instance is valid
	FAnimMontageInstance* theMontageInstance = GetMontageInstance(InAnimInstance, InMontage);
	if (theMontageInstance == nullptr)
	{
		return;
	}

	theMontageInstance->OnMontageEnded.Unbind();
	theMontageInstance->OnMontageEnded = nullptr;
}
#pragma endregion