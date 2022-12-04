// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "VMM_Types.h"
#include "DataAssets/VMM_LocomotionData.h"


/*-----------------------------------------------------------------------------
	FVirtualSkillParamsGroup implementation.
-----------------------------------------------------------------------------*/

const FAnimSkillParams* FVirtualSkillParamsGroup::GetSkillParams() const
{
	// Check the data index is valid
	if (DataIndex == INDEX_NONE)
	{
		return nullptr;
	}

	// Check the skill data asset is valid
	UVMM_LocomotionData* theSkillDataAsset = nullptr;
	if (theSkillDataAsset == nullptr)
	{
		return nullptr;
	}

	// Return the params reference
	return theSkillDataAsset->GetParamsByIndex(ParamsIndex);
}
