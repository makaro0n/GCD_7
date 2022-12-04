// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Abilities/VMM_GameplayAbility.h"
#include "Abilities/VMM_AbilitySystemComponent.h"
#include "Librarys/VMM_MontageLibrary.h"
#include "Input/VMM_InputAction.h"

UVMM_GameplayAbility::UVMM_GameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAnimInstance* UVMM_GameplayAbility::GetAnimInstanceFromActorInfo() const
{
	if (!ensure(CurrentActorInfo))
	{
		return nullptr;
	}

	// Check the self animation instance is valid, we return the result
	if (CurrentActorInfo->AnimInstance.IsValid())
	{
		return CurrentActorInfo->AnimInstance.Get();
	}

	// Check the ability system component is valid
	UAbilitySystemComponent* theAbilityComponent = GetAbilitySystemComponentFromActorInfo_Checked();
	if (theAbilityComponent == nullptr)
	{
		return nullptr;
	}

	// Return owner cached reference
	return theAbilityComponent->AbilityActorInfo.IsValid() ? theAbilityComponent->AbilityActorInfo->GetAnimInstance() : nullptr;
}

UMovementComponent* UVMM_GameplayAbility::GetMovementComponentFromActorInfo() const
{
	if (!ensure(CurrentActorInfo))
	{
		return nullptr;
	}

	return CurrentActorInfo->MovementComponent.Get();
}

UCharacterMovementComponent* UVMM_GameplayAbility::GetCharacterMovementComponentFromActorInfo() const
{
	return Cast<UCharacterMovementComponent>(GetMovementComponentFromActorInfo());
}

UVMM_CharacterMovementComponent* UVMM_GameplayAbility::GetVirtualCharacterMovementComponentFromActorInfo() const
{
	return Cast<UVMM_CharacterMovementComponent>(GetMovementComponentFromActorInfo());
}

#pragma region Input
const int32 UVMM_GameplayAbility::GetInputActionID() const
{
	return InputAction ? InputAction->GetInputID() : INDEX_NONE;
}
#pragma endregion


#pragma region Virtual Animation Skills
bool UVMM_GameplayAbility::PlaySkillMontage(const FVirtualSkillParamsGroup& InSkillParamsGroup, UAnimMontage*& OutPlayMontage, float InPlayRate, float InStartPosition)
{
	// Check the ability system component is valid
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent == nullptr)
	{
		return false;
	}

	// Play the skill params
	const float theMontageDuration = theAbilityComponent->PlaySkillMontage(InSkillParamsGroup, OutPlayMontage, InPlayRate, InStartPosition);

	// Define the play condition
	const bool bIsPlaying = theMontageDuration >= 0.f;
	if (bIsPlaying)
	{
		// Check the animation instance is valid
		UAnimInstance* theAnimInstance = theAbilityComponent->GetAnimInstance();
		check(theAnimInstance);

		// Bind montage blend out event
		MontageBlendOutDelegate.BindUObject(this, &ThisClass::OnAnimSkillMontageBlendOutEvent);
		theAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendOutDelegate, OutPlayMontage);

		// Bind montage blend ended event
		MontageBlendEndedDelegate.BindUObject(this, &ThisClass::OnAnimSkillMontageBlendEndedEvent);
		theAnimInstance->Montage_SetEndDelegate(MontageBlendEndedDelegate, OutPlayMontage);

		// Cache the params group data
		SkillParamsGroup = InSkillParamsGroup;
	}

	// Return the result
	return bIsPlaying;
}

bool UVMM_GameplayAbility::StopSkillMontage(UGameplayAbility* InAbility, FGameplayAbilityActivationInfo InActivationInfo, UAnimMontage* InMontage)
{
	// Check the ability system component is valid
	UVMM_AbilitySystemComponent* theAbilityComponent = Cast<UVMM_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	if (theAbilityComponent == nullptr)
	{
		return false;
	}

	// Stop the active montage
	theAbilityComponent->StopSkillMontage(this, GetCurrentActivationInfo(), InMontage);

	// Return the result
	return true;
}

void UVMM_GameplayAbility::OnClearAnimSkillRuntimeData(UAnimMontage* InMontage)
{
	UVMM_MontageLibrary::ClearMontageBindEvent(GetAnimInstanceFromActorInfo(), InMontage);
}
#pragma endregion
