// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "VMM_Types.h"
#include "CoreMinimal.h"
#include "Abilities/VMM_GameplayAbility.h"
#include "VMM_GameplayAbility_RotationMode.generated.h"

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_GameplayAbility_RotationMode : public UVMM_GameplayAbility
{
	GENERATED_UCLASS_BODY()

protected:

	/** Restore last rotation mode gameplay tag of camera */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FGameplayTag RestoreTag;

	/** Restore combat rotation mode gameplay tag of camera */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FGameplayTag RestoreCombatTag;

	/** Restore combat rotation mode gameplay tag of camera */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	ERotationMode RestoreCombatMode;

	/** Rotation mode gameplay tag of camera */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	TMap<ERotationMode, FGameplayTag> RotationModeMap;

public:

	/** Actually activate ability, do not call this directly */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle InHandle
		, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
		, const FGameplayEventData* InTriggerEventData) override;

	/** Native function, called if an ability ends normally or abnormally. If bReplicate is set to true, try to replicate the ending to the client/server */
	virtual void EndAbility(const FGameplayAbilitySpecHandle InHandle
		, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo
		, bool bReplicateEndAbility, bool bWasCancelled) override;

};
