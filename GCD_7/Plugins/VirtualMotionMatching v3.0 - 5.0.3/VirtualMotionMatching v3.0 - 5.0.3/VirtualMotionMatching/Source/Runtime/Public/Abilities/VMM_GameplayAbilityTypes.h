// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "VMM_GameplayAbilityTypes.generated.h"


#if 1 // Virtual Animation Skills

	/*-----------------------------------------------------------------------------
		It comes from the virtual animation skills plug-in.
		Once the two plug-ins need to be combined, we will focus on VAS.
	-----------------------------------------------------------------------------*/

/** Data about animation skill montages that is replicated to simulated clients */
USTRUCT()
struct FGameplayAbilityRepAnimSkillMontage
{
	GENERATED_USTRUCT_BODY()

	/** AnimMontage ref */
	UPROPERTY(NotReplicated)
	UAnimMontage* AnimMontage;

	/** Animation skill data index, It will search the skill data asset in the global manager */
	UPROPERTY()
	uint16 DataIndex;

	/** Animation skill params index, It will search the skill data asset in the global manager */
	UPROPERTY()
	uint8 ParamsIndex;

	/** Play Rate */
	UPROPERTY()
	float PlayRate;

	/** Montage position */
	UPROPERTY()
	float Position;

	/** Montage current blend time */
	UPROPERTY()
	float BlendTime;

	/** NextSectionID */
	UPROPERTY()
	uint8 NextSectionID;

	/** flag indicating we should serialize the position or the current section id */
	UPROPERTY()
	uint8 bRepPosition : 1;

	/** Bit set when montage has been stopped. */
	UPROPERTY()
	uint8 IsStopped : 1;

	/** Bit flipped every time a new Montage is played. To trigger replication when the same montage is played again. */
	UPROPERTY()
	uint8 ForcePlayBit : 1;

	/** Stops montage position from replicating at all to save bandwidth */
	UPROPERTY()
	uint8 SkipPositionCorrection : 1;

	/** Stops PlayRate from replicating to save bandwidth. PlayRate will be assumed to be 1.f. */
	UPROPERTY()
	uint8 bSkipPlayRate : 1;

	UPROPERTY()
	FPredictionKey PredictionKey;

public:

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	void SetRepAnimPositionMethod(ERepAnimPositionMethod InMethod);

	FGameplayAbilityRepAnimSkillMontage()
		: AnimMontage(nullptr)
		, DataIndex(0)
		, ParamsIndex(0)
		, PlayRate(0.f)
		, Position(0.f)
		, BlendTime(0.f)
		, NextSectionID(0)
		, bRepPosition(true)
		, IsStopped(true)
		, ForcePlayBit(0)
		, SkipPositionCorrection(false)
		, bSkipPlayRate(false)
	{}
};

#endif // Virtual Animation Skills