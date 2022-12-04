// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "VMM_Types.h"
#include "VMM_GameplayAbility.generated.h"

class UAnimInstance;
class UMovementComponent;
class UCharacterMovementComponent;
class UVMM_InputAction;

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_GameplayAbility : public UGameplayAbility
{
	GENERATED_UCLASS_BODY()

public:

	/** Convenience method for abilities to get animation instance - useful for aiming abilities */
	UFUNCTION(BlueprintCallable, DisplayName = "GetAnimInstanceFromActorInfo", Category = Ability)
	UAnimInstance* GetAnimInstanceFromActorInfo() const;

	/** Convenience method for abilities to get movement component - useful for aiming abilities */
	UFUNCTION(BlueprintCallable, DisplayName = "GetMovementComponentFromActorInfo", Category = Ability)
	UMovementComponent* GetMovementComponentFromActorInfo() const;

	/** Convenience method for abilities to get character movement component - useful for aiming abilities */
	UFUNCTION(BlueprintCallable, DisplayName = "GetCharacterMovementComponentFromActorInfo", Category = Ability)
	UCharacterMovementComponent* GetCharacterMovementComponentFromActorInfo() const;
	
	/** Convenience method for abilities to get character movement component - useful for aiming abilities */
	UFUNCTION(BlueprintCallable, DisplayName = "GetVirtualCharacterMovementComponentFromActorInfo", Category = Ability)
	UVMM_CharacterMovementComponent* GetVirtualCharacterMovementComponentFromActorInfo() const;


#pragma region Input

protected:

	/** Input action is used for input id. */
	UPROPERTY(EditDefaultsOnly, Category = Input)
	const UVMM_InputAction* InputAction;

public:

	/** Return virtual input action input id */
	const int32 GetInputActionID() const;

	/** Return virtual input action */
	FORCEINLINE const UVMM_InputAction* GetInputAction() const { return InputAction; }

#pragma endregion


#pragma region Virtual Animation Skills

protected:

	/** Data structure for montages that were instigated locally (everything if server, predictive if client. replicated if simulated proxy) */
	FVirtualSkillParamsGroup SkillParamsGroup;

	/** Plays a skill montage and handles replication and prediction based on passed in ability/activation info */
	virtual bool PlaySkillMontage(const FVirtualSkillParamsGroup& InSkillParamsGroup
		, UAnimMontage*& OutPlayMontage, float InPlayRate = 1.f, float InStartPosition = 0.f);

	/** Stop a skill montage and handles replication and prediction based on passed in ability/activation info */
	virtual bool StopSkillMontage(UGameplayAbility* InAbility, FGameplayAbilityActivationInfo InActivationInfo, UAnimMontage* InMontage);

protected:

	/** Cached montage blend out delegate */
	FOnMontageBlendingOutStarted MontageBlendOutDelegate;

	/** Cached montage blend ended delegate */
	FOnMontageEnded MontageBlendEndedDelegate;

	/** Clear last animation skill montage runtime data and bind event */
	virtual void OnClearAnimSkillRuntimeData(UAnimMontage* InMontage);

	/** Play Skill: Montage blend out event */
	UFUNCTION()
	virtual void OnAnimSkillMontageBlendOutEvent(UAnimMontage* InMontage, bool bInterrupted) {}

	/** Play Skill: Montage blend ended event */
	UFUNCTION()
	virtual void OnAnimSkillMontageBlendEndedEvent(UAnimMontage* InMontage, bool bInterrupted) {}

#pragma endregion

};
