// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Abilities/VMM_AbilitySystemComponent.h"
#include "Abilities/VMM_AbilitySet.h"
#include "Librarys/VMM_MontageLibrary.h"
#include "DataAssets/VMM_LocomotionData.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "VMM_Settings.h"

UAnimInstance* UVMM_AbilitySystemComponent::GetAnimInstance() const
{
	return AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
}

void UVMM_AbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
}

void UVMM_AbilitySystemComponent::InitializeAbilityData(AActor* InOwnerActor, AActor* InAvatarActor)
{
	if (AbilityData)
	{
		AbilityData->GiveToAbilitySystem(this, nullptr, InOwnerActor);
	}
}

void UVMM_AbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Fast Arrays don't use push model, but there's no harm in marking them with it.
	// The flag will just be ignored.

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UVMM_AbilitySystemComponent, RepAnimSkillMontageInfo, Params);

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

#if 1 // Virtual Animation Skills
void UVMM_AbilitySystemComponent::OnRep_ReplicatedAnimSkillMontage()
{
	const FGameplayAbilityRepAnimSkillMontage& theRepAnimMontageInfo = GetRepAnimSkillMontageInfo();

	if (AbilityActorInfo->IsNetAuthority())
	{
		return;
	}

	if (AbilityActorInfo->IsLocallyControlled())
	{
		return;
	}

	// Play the skill params
	UAnimMontage* thePlayMontage = nullptr;
	FVirtualSkillParamsGroup theSkillParamsGroup(int32(theRepAnimMontageInfo.DataIndex), int32(theRepAnimMontageInfo.ParamsIndex));
	PlaySkillMontage(theSkillParamsGroup, thePlayMontage, theRepAnimMontageInfo.PlayRate, theRepAnimMontageInfo.Position);

#if 0
	UWorld* World = GetWorld();

	const FGameplayAbilityRepAnimMontage& ConstRepAnimMontageInfo = GetRepAnimMontageInfo_Mutable();

	if (ConstRepAnimMontageInfo.bSkipPlayRate)
	{
		GetRepAnimMontageInfo_Mutable().PlayRate = 1.f;
	}

	const bool bIsPlayingReplay = World && World->IsPlayingReplay();

	const float MONTAGE_REP_POS_ERR_THRESH = bIsPlayingReplay ? CVarReplayMontageErrorThreshold.GetValueOnGameThread() : 0.1f;

	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr || !IsReadyForReplicatedMontage())
	{
		// We can't handle this yet
		bPendingMontageRep = true;
		return;
	}
	bPendingMontageRep = false;

	if (!AbilityActorInfo->IsLocallyControlled())
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("net.Montage.Debug"));
		bool DebugMontage = (CVar && CVar->GetValueOnGameThread() == 1);
		if (DebugMontage)
		{
			ABILITY_LOG(Warning, TEXT("\n\nOnRep_ReplicatedAnimMontage, %s"), *GetNameSafe(this));
			ABILITY_LOG(Warning, TEXT("\tAnimMontage: %s\n\tPlayRate: %f\n\tPosition: %f\n\tBlendTime: %f\n\tNextSectionID: %d\n\tIsStopped: %d\n\tForcePlayBit: %d"),
				*GetNameSafe(ConstRepAnimMontageInfo.AnimMontage),
				ConstRepAnimMontageInfo.PlayRate,
				ConstRepAnimMontageInfo.Position,
				ConstRepAnimMontageInfo.BlendTime,
				ConstRepAnimMontageInfo.NextSectionID,
				ConstRepAnimMontageInfo.IsStopped,
				ConstRepAnimMontageInfo.ForcePlayBit);
			ABILITY_LOG(Warning, TEXT("\tLocalAnimMontageInfo.AnimMontage: %s\n\tPosition: %f"),
				*GetNameSafe(LocalAnimMontageInfo.AnimMontage), AnimInstance->Montage_GetPosition(LocalAnimMontageInfo.AnimMontage));
		}

		if (ConstRepAnimMontageInfo.AnimMontage)
		{
			// New Montage to play
			const bool ReplicatedPlayBit = bool(ConstRepAnimMontageInfo.ForcePlayBit);
			if ((LocalAnimMontageInfo.AnimMontage != ConstRepAnimMontageInfo.AnimMontage) || (LocalAnimMontageInfo.PlayBit != ReplicatedPlayBit))
			{
				LocalAnimMontageInfo.PlayBit = ReplicatedPlayBit;
				PlayMontageSimulated(ConstRepAnimMontageInfo.AnimMontage, ConstRepAnimMontageInfo.PlayRate);
			}

			if (LocalAnimMontageInfo.AnimMontage == nullptr)
			{
				ABILITY_LOG(Warning, TEXT("OnRep_ReplicatedAnimMontage: PlayMontageSimulated failed. Name: %s, AnimMontage: %s"), *GetNameSafe(this), *GetNameSafe(ConstRepAnimMontageInfo.AnimMontage));
				return;
			}

			// Play Rate has changed
			if (AnimInstance->Montage_GetPlayRate(LocalAnimMontageInfo.AnimMontage) != ConstRepAnimMontageInfo.PlayRate)
			{
				AnimInstance->Montage_SetPlayRate(LocalAnimMontageInfo.AnimMontage, ConstRepAnimMontageInfo.PlayRate);
			}

			const int32 SectionIdToPlay = (static_cast<int32>(ConstRepAnimMontageInfo.SectionIdToPlay) - 1);
			if (SectionIdToPlay != INDEX_NONE)
			{
				FName SectionNameToJumpTo = LocalAnimMontageInfo.AnimMontage->GetSectionName(SectionIdToPlay);
				if (SectionNameToJumpTo != NAME_None)
				{
					AnimInstance->Montage_JumpToSection(SectionNameToJumpTo);
				}
			}

			// Compressed Flags
			const bool bIsStopped = AnimInstance->Montage_GetIsStopped(LocalAnimMontageInfo.AnimMontage);
			const bool bReplicatedIsStopped = bool(ConstRepAnimMontageInfo.IsStopped);

			// Process stopping first, so we don't change sections and cause blending to pop.
			if (bReplicatedIsStopped)
			{
				if (!bIsStopped)
				{
					CurrentMontageStop(ConstRepAnimMontageInfo.BlendTime);
				}
			}
			else if (!ConstRepAnimMontageInfo.SkipPositionCorrection)
			{
				const int32 RepSectionID = LocalAnimMontageInfo.AnimMontage->GetSectionIndexFromPosition(ConstRepAnimMontageInfo.Position);
				const int32 RepNextSectionID = int32(ConstRepAnimMontageInfo.NextSectionID) - 1;

				// And NextSectionID for the replicated SectionID.
				if (RepSectionID != INDEX_NONE)
				{
					const int32 NextSectionID = AnimInstance->Montage_GetNextSectionID(LocalAnimMontageInfo.AnimMontage, RepSectionID);

					// If NextSectionID is different than the replicated one, then set it.
					if (NextSectionID != RepNextSectionID)
					{
						AnimInstance->Montage_SetNextSection(LocalAnimMontageInfo.AnimMontage->GetSectionName(RepSectionID), LocalAnimMontageInfo.AnimMontage->GetSectionName(RepNextSectionID), LocalAnimMontageInfo.AnimMontage);
					}

					// Make sure we haven't received that update too late and the client hasn't already jumped to another section. 
					const int32 CurrentSectionID = LocalAnimMontageInfo.AnimMontage->GetSectionIndexFromPosition(AnimInstance->Montage_GetPosition(LocalAnimMontageInfo.AnimMontage));
					if ((CurrentSectionID != RepSectionID) && (CurrentSectionID != RepNextSectionID))
					{
						// Client is in a wrong section, teleport him into the begining of the right section
						const float SectionStartTime = LocalAnimMontageInfo.AnimMontage->GetAnimCompositeSection(RepSectionID).GetTime();
						AnimInstance->Montage_SetPosition(LocalAnimMontageInfo.AnimMontage, SectionStartTime);
					}
				}

				// Update Position. If error is too great, jump to replicated position.
				const float CurrentPosition = AnimInstance->Montage_GetPosition(LocalAnimMontageInfo.AnimMontage);
				const int32 CurrentSectionID = LocalAnimMontageInfo.AnimMontage->GetSectionIndexFromPosition(CurrentPosition);
				const float DeltaPosition = ConstRepAnimMontageInfo.Position - CurrentPosition;

				// Only check threshold if we are located in the same section. Different sections require a bit more work as we could be jumping around the timeline.
				// And therefore DeltaPosition is not as trivial to determine.
				if ((CurrentSectionID == RepSectionID) && (FMath::Abs(DeltaPosition) > MONTAGE_REP_POS_ERR_THRESH) && (ConstRepAnimMontageInfo.IsStopped == 0))
				{
					// fast forward to server position and trigger notifies
					if (FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(ConstRepAnimMontageInfo.AnimMontage))
					{
						// Skip triggering notifies if we're going backwards in time, we've already triggered them.
						const float DeltaTime = !FMath::IsNearlyZero(ConstRepAnimMontageInfo.PlayRate) ? (DeltaPosition / ConstRepAnimMontageInfo.PlayRate) : 0.f;
						if (DeltaTime >= 0.f)
						{
							MontageInstance->UpdateWeight(DeltaTime);
							MontageInstance->HandleEvents(CurrentPosition, ConstRepAnimMontageInfo.Position, nullptr);
							AnimInstance->TriggerAnimNotifies(DeltaTime);
						}
					}
					AnimInstance->Montage_SetPosition(LocalAnimMontageInfo.AnimMontage, ConstRepAnimMontageInfo.Position);
				}
			}
		}
	}
#endif
}

void UVMM_AbilitySystemComponent::SetRepAnimSkillMontageInfo(const FGameplayAbilityRepAnimSkillMontage& InData)
{
	GetRepAnimSkillMontageInfo_Mutable() = InData;
}

FGameplayAbilityRepAnimSkillMontage& UVMM_AbilitySystemComponent::GetRepAnimSkillMontageInfo_Mutable()
{
	MARK_PROPERTY_DIRTY_FROM_NAME(UVMM_AbilitySystemComponent, RepAnimSkillMontageInfo, this);
	return RepAnimSkillMontageInfo;
}

float UVMM_AbilitySystemComponent::PlaySkillMontage(const FVirtualSkillParamsGroup& InSkillParamsGroup
	, UAnimMontage*& OutPlayMontage, float InPlayRate, float InStartPosition)
{
	// Define the montage duration
	float theMontageDuration = -1.f;

	// Check the asset manager is valid
	const UVMM_Settings* theMutableDefaultSettings = GetDefault<UVMM_Settings>();
	if (theMutableDefaultSettings == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_AbilitySystemComponent::PlaySkillMontage, Invalid global manager, DataIndex: %d, ParamsIndex: %d")
			, InSkillParamsGroup.DataIndex, InSkillParamsGroup.ParamsIndex);
		return theMontageDuration;
	}

// 	FString CustomConfigIni;
// 	FConfigCacheIni::LoadGlobalIniFile(CustomConfigIni, *GetConfigFilename(theMutableDefaultSettings->ConfigFileName), nullptr, true);
// 	LoadConfig(NULL, NULL, UE4::LCPF_PropagateToChildDefaultObjects, NULL);

	// Check the skill data asset is valid
	const UVMM_LocomotionData* theSkillData = theMutableDefaultSettings->GetDataAsset(InSkillParamsGroup.DataIndex);
	if (theSkillData == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_AbilitySystemComponent::PlaySkillMontage, Invalid data, DataIndex: %d, ParamsIndex: %d")
			, InSkillParamsGroup.DataIndex, InSkillParamsGroup.ParamsIndex);
		return theMontageDuration;
	}

	// Check the skill params is valid
	const FAnimSkillParams* theSkillParams = theSkillData->GetParamsByIndex(InSkillParamsGroup.ParamsIndex);
	if (theSkillParams == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_AbilitySystemComponent::PlaySkillMontage, Invalid params, DataIndex: %d, ParamsIndex: %d")
			, InSkillParamsGroup.DataIndex, InSkillParamsGroup.ParamsIndex);
		return theMontageDuration;
	}

	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = GetAnimInstance();
	if (theAnimInstance == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_AbilitySystemComponent::PlaySkillMontage, Invalid animation instance"));
		return theMontageDuration;
	}

	// Check the dynamic montage is valid
	UAnimMontage* thePlayMontage = OutPlayMontage == nullptr ? UVMM_MontageLibrary::CreateSlotAnimationAsDynamicMontage(theAnimInstance, theSkillParams) : OutPlayMontage;
	if (thePlayMontage == nullptr)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_AbilitySystemComponent::PlaySkillMontage, Invalid Montage"));
		return theMontageDuration;
	}

	// Play the montage
	OutPlayMontage = thePlayMontage;
	theMontageDuration = theAnimInstance->Montage_Play(thePlayMontage, InPlayRate, EMontagePlayReturnType::MontageLength, InStartPosition);
	
	// Check the montage play duration is valid
	if (theMontageDuration <= 0.f)
	{
		return theMontageDuration;
	}

	// Cache the params data
	SkillParamsGroup = InSkillParamsGroup;

#if 1
// 	if (InAnimatingAbility)
// 	{
// 		InAnimatingAbility->SetCurrentMontage(NewAnimMontage);
// 	}

	// Replicate to non owners
	if (IsOwnerActorAuthoritative())
	{
		// Get mutable animation skill montage info
		FGameplayAbilityRepAnimSkillMontage& theRepAnimSkillMontageInfo = GetRepAnimSkillMontageInfo_Mutable();

		// Those are static parameters, they are only set when the montage is played. They are not changed after that.
		theRepAnimSkillMontageInfo.AnimMontage = thePlayMontage;
		theRepAnimSkillMontageInfo.ForcePlayBit = !bool(theRepAnimSkillMontageInfo.ForcePlayBit);

		// Update parameters that change during Montage life time.
		theRepAnimSkillMontageInfo.DataIndex = InSkillParamsGroup.DataIndex;
		theRepAnimSkillMontageInfo.ParamsIndex = InSkillParamsGroup.ParamsIndex;
		theRepAnimSkillMontageInfo.PlayRate = InPlayRate;
		theRepAnimSkillMontageInfo.Position = InStartPosition;
		theRepAnimSkillMontageInfo.BlendTime = thePlayMontage->BlendIn.GetBlendTime();

		// Force net update on our avatar actor
		if (AbilityActorInfo->AvatarActor != nullptr)
		{
			AbilityActorInfo->AvatarActor->ForceNetUpdate();
		}
	}
	else if(AbilityActorInfo->IsLocallyControlled())
	{
		// Make the animation skill montage info
		FGameplayAbilityRepAnimSkillMontage theRepAnimSkillMontageInfo;
		theRepAnimSkillMontageInfo.AnimMontage = thePlayMontage;
		theRepAnimSkillMontageInfo.ForcePlayBit = !bool(theRepAnimSkillMontageInfo.ForcePlayBit);
		theRepAnimSkillMontageInfo.DataIndex = InSkillParamsGroup.DataIndex;
		theRepAnimSkillMontageInfo.ParamsIndex = InSkillParamsGroup.ParamsIndex;
		theRepAnimSkillMontageInfo.PlayRate = InPlayRate;
		theRepAnimSkillMontageInfo.Position = InStartPosition;
		theRepAnimSkillMontageInfo.BlendTime = thePlayMontage->BlendIn.GetBlendTime();

		ServerPlaySkillMontage(theRepAnimSkillMontageInfo);
#if 0
		// If this prediction key is rejected, we need to end the preview
		FPredictionKey PredictionKey = GetPredictionKeyForNewAction();
		if (PredictionKey.IsValidKey())
		{
			PredictionKey.NewRejectedDelegate().BindUObject(this, &UAbilitySystemComponent::OnPredictiveMontageRejected, NewAnimMontage);
		}
#endif
	}
#endif

	// Return default result
	return theMontageDuration;
}

void UVMM_AbilitySystemComponent::StopSkillMontage(UGameplayAbility* InAbility, FGameplayAbilityActivationInfo InActivationInfo, UAnimMontage* InMontage)
{
	// Check the montage is valid
	if (InMontage == NULL)
	{
		return;
	}

	// Check the animation instance is valid
	UAnimInstance* theAnimInstance = GetAnimInstance();
	if (theAnimInstance == nullptr)
	{
		return;
	}

	// Stop the montage
	UVMM_MontageLibrary::MontageStop(theAnimInstance, InMontage);
}

void UVMM_AbilitySystemComponent::ServerPlaySkillMontage_Implementation(FGameplayAbilityRepAnimSkillMontage InSkillMontageInfo)
{
	// Play the skill params
	UAnimMontage* thePlayMontage = nullptr;
	FVirtualSkillParamsGroup theSkillParamsGroup(int32(InSkillMontageInfo.DataIndex), int32(InSkillMontageInfo.ParamsIndex));
	PlaySkillMontage(theSkillParamsGroup, thePlayMontage, InSkillMontageInfo.PlayRate, InSkillMontageInfo.Position);
}

bool UVMM_AbilitySystemComponent::ServerPlaySkillMontage_Validate(FGameplayAbilityRepAnimSkillMontage InSkillMontageInfo)
{
	return true;
}
#endif // Virtual Animation Skills
