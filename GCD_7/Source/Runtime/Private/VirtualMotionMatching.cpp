// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "VirtualMotionMatching.h"

DEFINE_LOG_CATEGORY(LogVirtualMotionMatching);

#define LOCTEXT_NAMESPACE "FVirtualMotionMatchingModule"

void FVirtualMotionMatchingModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FVirtualMotionMatchingModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVirtualMotionMatchingModule, VirtualMotionMatching)