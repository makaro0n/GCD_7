// Fill out your copyright notice in the Description page of Project Settings.


#include "Abilities/VMM_GameplayAbility_RotationMode.h"
#include "GameFramework/VMM_Character.h"
#include "Player/VMM_PlayerController.h"
#include "Player/VMM_PlayerCameraManager.h"
#include "VirtualMotionMatching.h"


UVMM_GameplayAbility_RotationMode::UVMM_GameplayAbility_RotationMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Initialize the rotation mode map
	RestoreCombatMode = ERotationMode::UnEasy;
	for (int32 i = 0; i < int32(ERotationMode::MAX); i++)
	{
		RotationModeMap.FindOrAdd(ERotationMode(i));
	}
}

void UVMM_GameplayAbility_RotationMode::ActivateAbility(const FGameplayAbilitySpecHandle InHandle
	, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
	, const FGameplayEventData* InTriggerEventData)
{
	if (!CommitAbility(InHandle, InActorInfo, InActivationInfo))
	{
		EndAbility(InHandle, InActorInfo, InActivationInfo, true, false);
		return;
	}

	// Check the avatar character is valid
	AVMM_Character* theAvatarCharacter = Cast<AVMM_Character>(InActorInfo->AvatarActor.Get());
	if (theAvatarCharacter == nullptr)
	{
		EndAbility(InHandle, InActorInfo, InActivationInfo, true, false);
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_RotationMode::ActivateAbility, Invalid avatar character."));
		return;
	}

	// Define the apply state
	bool bApply = false;

	// If none is not specified mode, we should select down
	if (InTriggerEventData == nullptr)
	{
		// Get current view mode
		const ERotationMode& theRotationMode = theAvatarCharacter->GetRotationMode();

		// Additive the rotation mode index
		ERotationMode theNextRotationMode = ERotationMode(uint8(theRotationMode) + 1);

		// If the next view mode is invalid, we should reset the index
		if (theNextRotationMode == ERotationMode::MAX)
		{
			theNextRotationMode = ERotationMode(0);
		}

		// Apply the new rotation mode
		bApply = theAvatarCharacter->SetRotationMode(theNextRotationMode);
	}
	else if (InTriggerEventData->EventTag == RestoreTag)
	{
		// Apply the last looking rotation mode
		bApply = theAvatarCharacter->SetRotationMode(theAvatarCharacter->GetLastLookingRotationMode());
	}
	else if (InTriggerEventData->EventTag == RestoreCombatTag)
	{
		// Restore combat rotation mode
		if (theAvatarCharacter->IsCombatRotationMode())
		{
			bApply = theAvatarCharacter->SetRotationMode(RestoreCombatMode);
		}
	}
	else
	{
		// Find desired view mode
		for (TPair<ERotationMode, FGameplayTag>& theRotationModePair : RotationModeMap)
		{
			// Check the tag is valid
			if (theRotationModePair.Value != InTriggerEventData->EventTag)
			{
				continue;
			}

			// Check the released velocity rotation mode
			if (theRotationModePair.Key == ERotationMode::Velocity && theAvatarCharacter->IsVelocityRotationMode())
			{
				// Apply the last looking rotation mode
				bApply = theAvatarCharacter->SetRotationMode(theAvatarCharacter->GetLastLookingRotationMode());
				break;
			}

			// We don't allow combat mode enter to uneasy mode, should use restore
			if (theRotationModePair.Key == ERotationMode::UnEasy && theAvatarCharacter->IsCombatRotationMode())
			{
				break;
			}

			// Apply the new rotation mode
			bApply = theAvatarCharacter->SetRotationMode(theRotationModePair.Key);
			break;
		}
	}

	// If apply the rotation mode, we will notify camera manager
	if (bApply && InActorInfo->IsLocallyControlledPlayer())
	{
		if (const FGameplayTag* theGameplayTag = RotationModeMap.Find(theAvatarCharacter->GetRotationMode()))
		{
			if (APlayerController* thePlayerController = InActorInfo->PlayerController.Get())
			{
				if (AVMM_PlayerCameraManager* thePlayerCameraManager = Cast<AVMM_PlayerCameraManager>(thePlayerController->PlayerCameraManager))
				{
					thePlayerCameraManager->SetCameraSettings(*theGameplayTag);
				}
			}
		}
	}

	// End the ability
	EndAbility(InHandle, InActorInfo, InActivationInfo, true, false);
}

void UVMM_GameplayAbility_RotationMode::EndAbility(const FGameplayAbilitySpecHandle InHandle
	, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
	, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(InHandle, InActorInfo, InActivationInfo, bReplicateEndAbility, bWasCancelled);
}
