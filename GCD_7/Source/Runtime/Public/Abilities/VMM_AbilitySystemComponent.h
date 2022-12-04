// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "VMM_Types.h"
#include "VMM_GameplayAbilityTypes.h"
#include "VMM_AbilitySystemComponent.generated.h"

class UVMM_AbilitySet;

/**
 * 
 */
UCLASS(ClassGroup = AbilitySystem, hidecategories = (Object, LOD, Lighting, Transform, Sockets, TextureStreaming), editinlinenew, meta = (BlueprintSpawnableComponent))
class VIRTUALMOTIONMATCHING_API UVMM_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

protected:

	/** Ability data test */
	UPROPERTY(EditDefaultsOnly, Category = "AttributeTest")
	UVMM_AbilitySet* AbilityData;

public:

	/** Return animation instance reference */
	UAnimInstance* GetAnimInstance() const;

	/**
	 *	Initialized the Abilities' ActorInfo - the structure that holds information about who we are acting on and who controls us.
	 *      OwnerActor is the actor that logically owns this component.
	 *		AvatarActor is what physical actor in the world we are acting on. Usually a Pawn but it could be a Tower, Building, Turret, etc, may be the same as Owner
	 */
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	/** Initialize ability data */
	virtual void InitializeAbilityData(AActor* InOwnerActor, AActor* InAvatarActor);

#if 1 // Virtual Animation Skills

	/*-----------------------------------------------------------------------------
		It comes from the virtual animation skills plug-in. 
		Once the two plug-ins need to be combined, we will focus on VAS.
	-----------------------------------------------------------------------------*/

protected:

	/** Data structure for montages that were instigated locally (everything if server, predictive if client. replicated if simulated proxy) */
	FVirtualSkillParamsGroup SkillParamsGroup;

	/** Data structure for replicating animation skill montage info to simulated clients */
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedAnimSkillMontage)
	FGameplayAbilityRepAnimSkillMontage RepAnimSkillMontageInfo;

	/** Replicated animation skill montage info callback */
	UFUNCTION()
	virtual void OnRep_ReplicatedAnimSkillMontage();

	/** Set the replicated animation skill montage info */
	void SetRepAnimSkillMontageInfo(const FGameplayAbilityRepAnimSkillMontage& InData);

	/** Return the mutable replicated animation skill montage info */
	FGameplayAbilityRepAnimSkillMontage& GetRepAnimSkillMontageInfo_Mutable();

	/** Return the const  replicated animation skill montage info */
	FORCEINLINE const FGameplayAbilityRepAnimSkillMontage& GetRepAnimSkillMontageInfo() const { return RepAnimSkillMontageInfo; }

public:

	/** Plays a skill montage and handles replication and prediction based on passed in ability/activation info */
	virtual float PlaySkillMontage(const FVirtualSkillParamsGroup& InSkillParamsGroup
		, UAnimMontage*& OutPlayMontage, float InPlayRate = 1.f, float InStartPosition = 0.f);

	/** Stop a skill montage and handles replication and prediction based on passed in ability/activation info */
	virtual void StopSkillMontage(UGameplayAbility* InAbility, FGameplayAbilityActivationInfo InActivationInfo, UAnimMontage* InMontage);

protected:

	/** Ask the server to send animation skill montage data, @See::PlaySkillMontage  */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPlaySkillMontage(FGameplayAbilityRepAnimSkillMontage InSkillMontageInfo);

public:

	/** Return current skill montage */
	FORCEINLINE UAnimMontage* GetSkillMontage() const { return RepAnimSkillMontageInfo.AnimMontage; }

	/** Return current skill params group */
	FORCEINLINE const FVirtualSkillParamsGroup& GetSkillParamsGroup() const { return SkillParamsGroup; }

#endif // Virtual Animation Skills
};
