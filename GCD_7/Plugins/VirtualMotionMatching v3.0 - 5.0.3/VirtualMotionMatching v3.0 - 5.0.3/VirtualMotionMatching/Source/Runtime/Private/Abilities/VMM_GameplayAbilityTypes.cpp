// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Abilities/VMM_GameplayAbilityTypes.h"


/*-----------------------------------------------------------------------------
	FGameplayAbilityRepAnimSkillMontage implementation.
-----------------------------------------------------------------------------*/

bool FGameplayAbilityRepAnimSkillMontage::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 RepPosition = bRepPosition;
	Ar.SerializeBits(&RepPosition, 1);
	if (RepPosition)
	{
		bRepPosition = true;

		// when rep'ing position, we don't want to skip correction
		// and we don't need to force the section id to play
		//SectionIdToPlay = 0;
		SkipPositionCorrection = false;

		// @note: section frames have such a high amount of precision they use, when
		// removing some of the position precision and packing it into a uint32 caused
		// issues where ability code would pick the end of a previous section instead of
		// the start of a new section. For now serializing the full position again.
		Ar << Position;
	}
	else
	{
		bRepPosition = false;

		// when rep'ing the section to play id, we want to skip
		// correction, and don't want a position
		SkipPositionCorrection = true;
		Position = 0.0f;
		//Ar.SerializeBits(&SectionIdToPlay, 7);
	}

	uint8 bIsStopped = IsStopped;
	Ar.SerializeBits(&bIsStopped, 1);
	IsStopped = bIsStopped & 1;

	uint8 bForcePlayBit = ForcePlayBit;
	Ar.SerializeBits(&bForcePlayBit, 1);
	ForcePlayBit = bForcePlayBit & 1;

	uint8 bSkipPositionCorrection = SkipPositionCorrection;
	Ar.SerializeBits(&bSkipPositionCorrection, 1);
	SkipPositionCorrection = bSkipPositionCorrection & 1;

	uint8 SkipPlayRate = bSkipPlayRate;
	Ar.SerializeBits(&SkipPlayRate, 1);
	bSkipPlayRate = SkipPlayRate & 1;

	Ar << DataIndex;
	Ar << ParamsIndex;
	Ar << PlayRate;
	Ar << BlendTime;
	Ar << NextSectionID;
	PredictionKey.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

void FGameplayAbilityRepAnimSkillMontage::SetRepAnimPositionMethod(ERepAnimPositionMethod InMethod)
{
	switch (InMethod)
	{
	case ERepAnimPositionMethod::Position:
		bRepPosition = true;
		break;

	case ERepAnimPositionMethod::CurrentSectionId:
		bRepPosition = false;
		break;
	}
}
