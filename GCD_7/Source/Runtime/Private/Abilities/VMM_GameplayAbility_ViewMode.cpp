// Fill out your copyright notice in the Description page of Project Settings.


#include "Abilities/VMM_GameplayAbility_ViewMode.h"
#include "Player/VMM_PlayerCameraManager.h"
#include "VirtualMotionMatching.h"

UVMM_GameplayAbility_ViewMode::UVMM_GameplayAbility_ViewMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Initialize the view mode map
	for (int32 i = 0; i < int32(ECameraViewMode::MAX) ; i++)
	{
		ViewModeMap.FindOrAdd(ECameraViewMode(i));
	}
}

void UVMM_GameplayAbility_ViewMode::ActivateAbility(const FGameplayAbilitySpecHandle InHandle
	, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
	, const FGameplayEventData* InTriggerEventData)
{
	check(InActorInfo);

	// Check the movement component is valid
	APlayerCameraManager* theSourcePlayerCameraManager = InActorInfo->PlayerController.IsValid() ? InActorInfo->PlayerController.Get()->PlayerCameraManager : nullptr;
	AVMM_PlayerCameraManager* thePlayerCameraManager = Cast<AVMM_PlayerCameraManager>(theSourcePlayerCameraManager);
	if (thePlayerCameraManager == nullptr)
	{
		EndAbility(InHandle, InActorInfo, InActivationInfo, true, false);
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_GameplayAbility_ViewMode::ActivateAbility, Invalid player camera manager."));
		return;
	}

	// If none is not specified mode, we should select down
	if (InTriggerEventData == nullptr)
	{
		// Get current view mode
		const ECameraViewMode& theViewMode = thePlayerCameraManager->GetViewMode();

		// Additive the view mode index
		ECameraViewMode theNextViewMode = ECameraViewMode(uint8(theViewMode) + 1);

		// If the next view mode is invalid, we should reset the index
		if (theNextViewMode == ECameraViewMode::MAX)
		{
			theNextViewMode = ECameraViewMode(0);
		}
		else if (theNextViewMode == ECameraViewMode::TopDownPerson) // Ignore top down person now
		{
			theNextViewMode = ECameraViewMode(0);
		}

		// Toggle the view mode
		thePlayerCameraManager->SetViewMode(theNextViewMode, ViewModeMap.Find(theNextViewMode));
	}
	else
	{
		// Find desired view mode
		for (TPair<ECameraViewMode, FGameplayTag>& theViewModePair : ViewModeMap)
		{
			// Check the tag is valid
			if (theViewModePair.Value != InTriggerEventData->EventTag)
			{
				continue;
			}

			// Toggle the view mode
			thePlayerCameraManager->SetViewMode(theViewModePair.Key, &theViewModePair.Value);
			break;
		}
	}

	// End the ability
	EndAbility(InHandle, InActorInfo, InActivationInfo, true, false);
}

void UVMM_GameplayAbility_ViewMode::EndAbility(const FGameplayAbilitySpecHandle InHandle
	, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
	, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(InHandle, InActorInfo, InActivationInfo, bReplicateEndAbility, bWasCancelled);
}
