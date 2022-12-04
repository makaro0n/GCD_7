// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVirtualMotionMatching, Log, All);
DECLARE_STATS_GROUP(TEXT("Virtual Motion Matching"), STATGROUP_VirtualMotionMatching, STATCAT_Advanced);

class FVirtualMotionMatchingModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
