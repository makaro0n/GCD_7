// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Librarys/VMM_Library.h"
#include "VirtualMotionMatching.h"

#include "DataRegistry/Public/DataRegistry.h"
#include "DataRegistry/Public/DataRegistrySubsystem.h"
#include "DataRegistry/Public/DataRegistrySource.h"
#include "DataRegistry/Public/DataRegistrySource_DataTable.h"


#pragma region Network
FString UVMM_Library::GetRoleName(const AActor* InActor)
{
	// Check the actor is valid
	if (InActor == nullptr)
	{
		return FString();
	}

	// Return current role string
	if (UEnum* theRolePtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ENetRole"), true))
	{
		const ENetRole theRole = InActor->GetLocalRole();
		return theRolePtr->GetNameStringByValue(theRole);
	}

	// INVALID
	return FString();
}

bool UVMM_Library::IsAuthority(const AActor* InActor)
{
	// Check the actor is valid
	if (InActor == nullptr)
	{
		return false;
	}

	// Return the result
	return InActor->GetLocalRole() == ROLE_Authority;
}

bool UVMM_Library::IsAutonomousProxy(const AActor* InActor)
{
	// Check the actor is valid
	if (InActor == nullptr)
	{
		return false;
	}

	// Return the result
	if (InActor->GetLocalRole() == ROLE_AutonomousProxy)
	{
		return true;
	}
	else if (AActor* theOwner = InActor->GetOwner())
	{
		return theOwner->GetLocalRole() == ROLE_AutonomousProxy;
	}

	// INVALID
	return false;
}

bool UVMM_Library::IsSimulatedProxy(const AActor* InActor)
{
	// Check the actor is valid
	if (InActor == nullptr)
	{
		return false;
	}

	// Return the result
	return InActor->GetLocalRole() == ROLE_SimulatedProxy;
}
#pragma endregion

bool UVMM_Library::WaitTime(const float& InDeltaSeconds, float &InValue, const float InWaitTime)
{
	// Check the max wait time is valid
	if (InWaitTime <= 0.f)
	{
		return true;
	}

	// Accumulate wait time
	InValue += InDeltaSeconds;

	// Return the result
	return InValue >= InWaitTime;
}

#pragma region Data Registry
bool UVMM_Library::GetRegistryData(const FDataRegistryId& InRegistryId, void* OutData)
{
	// Check the data registry subsystem is valid
	UDataRegistrySubsystem* theSubSystem = UDataRegistrySubsystem::Get();
	if (!theSubSystem)
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_Library::GetItemRegistryData, Invalid data registry subsystem!"));
		return false;
	}

	// Check the subsystem enable state
	if (!theSubSystem->IsConfigEnabled(true))
	{
		UE_LOG(LogVirtualMotionMatching, Error, TEXT("UVMM_Library::GetItemRegistryData, Disable subsystem config state!"));
		return false;
	}

	// Define the cache data memory
	const uint8* theCacheData = nullptr;

	// Define the cache struct memory
	const UScriptStruct* theCacheStruct = nullptr;

	// Define the cache result
	FDataRegistryCacheGetResult theDataRegistryCacheResult;

	// Define the default result is not found
	EDataRegistrySubsystemGetItemResult OutResult = EDataRegistrySubsystemGetItemResult::NotFound;

	// Find the cached item raw data
	P_NATIVE_BEGIN;
	theDataRegistryCacheResult = theSubSystem->GetCachedItemRaw(theCacheData, theCacheStruct, InRegistryId);

	// Check the cache result data is valid
	if (!theDataRegistryCacheResult)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_Library::GetItemRegistryData, Invalid data registry cache result: %s"), *InRegistryId.ToString());
		return false;
	}

	// Check the cache struct data is valid
	if (!theCacheStruct)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_Library::GetItemRegistryData, Invalid data registry cache struct: %s"), *InRegistryId.ToString());
		return false;
	}

	// Check the cache data is valid
	if (!theCacheData)
	{
		UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_Library::GetItemRegistryData, Invalid data registry cache data: %s"), *InRegistryId.ToString());
		return false;
	}

	// Set the result is found
	OutResult = EDataRegistrySubsystemGetItemResult::Found;

	// Copy the struct data
	theCacheStruct->CopyScriptStruct(OutData, theCacheData);
	P_NATIVE_END;

	return true;
}
#pragma endregion


#pragma region Inpu Direction
EInputDirection UVMM_Library::GetInputDirectionType(const float InAngle, const float InTolerance)
{
	if (FMath::IsWithinInclusive(InAngle, 0.f - InTolerance, 0.f + InTolerance))
	{
		return EInputDirection::Forward;
	}
	else if (InAngle > 0.f)
	{
		if (FMath::IsWithinInclusive(InAngle, 0.f + InTolerance, 45.f + InTolerance))
		{
			return EInputDirection::ForwardRight;
		}
		else if (FMath::IsWithinInclusive(InAngle, 45.f + InTolerance, 90.f + InTolerance))
		{
			return EInputDirection::Right;
		}
		else if (FMath::IsWithinInclusive(InAngle, 90.f + InTolerance, 135.f + InTolerance))
		{
			return EInputDirection::BackwardRight;
		}
	}
	else
	{
		if (FMath::IsWithinInclusive(InAngle, -45.f - InTolerance, 0.f - InTolerance))
		{
			return EInputDirection::ForwardLeft;
		}
		else if (FMath::IsWithinInclusive(InAngle, -90.f - InTolerance, -45.f - InTolerance))
		{
			return EInputDirection::Left;
		}
		else if (FMath::IsWithinInclusive(InAngle, -135.f - InTolerance, -90.f - InTolerance))
		{
			return EInputDirection::BackwardLeft;
		}
	}

	// Return default result
	return EInputDirection::Backward;
}
#pragma endregion


#pragma region Angle Direction
float UVMM_Library::GetAngleDirectionValue(const EAngleDirection InAngleDirection)
{
	switch (InAngleDirection)
	{
	case EAngleDirection::ForwardLF:
		return 0.f;

	case EAngleDirection::ForwardRF:
		return 0.f;

	case EAngleDirection::ForwardLeft:
		return -45.f;

	case EAngleDirection::ForwardRight:
		return 45.f;

	case EAngleDirection::Left:
		return -90.f;

	case EAngleDirection::Right:
		return 90.f;

	case EAngleDirection::BackwardLeft:
		return -135.f;

	case EAngleDirection::BackwardRight:
		return 135.f;

	case EAngleDirection::BackwardLF:
		return -180.f;

	case EAngleDirection::BackwardRF:
		return 180.f;
	}

	// Return default value
	return 0.f;
}


FString UVMM_Library::GetAngleDirectionString(const EAngleDirection& InDirection)
{
	// Check the enum is valid
	if (UEnum* theEnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAngleDirection"), true))
	{
		return theEnumPtr->GetNameStringByValue(int64(InDirection));
	}

	// Invalid result
	return TEXT("INVALID");
}

EAngleDirection UVMM_Library::GetAngleDirectionType(const float InAngle, const float InTolerance)
{
	if (FMath::IsWithinInclusive(InAngle, 0.f - InTolerance, 0.f + InTolerance))
	{
		return InAngle > 0.f ? EAngleDirection::ForwardRF : EAngleDirection::ForwardLF;
	}
	else if (InAngle > 0.f)
	{
		if (FMath::IsWithinInclusive(InAngle, 0.f + InTolerance, 45.f + InTolerance))
		{
			return EAngleDirection::ForwardRight;
		}
		else if (FMath::IsWithinInclusive(InAngle, 45.f + InTolerance, 90.f + InTolerance))
		{
			return EAngleDirection::Right;
		}
		else if (FMath::IsWithinInclusive(InAngle, 90.f + InTolerance, 135.f + InTolerance))
		{
			return EAngleDirection::BackwardRight;
		}
	}
	else
	{
		if (FMath::IsWithinInclusive(InAngle, -45.f - InTolerance, 0.f - InTolerance))
		{
			return EAngleDirection::ForwardLeft;
		}
		else if (FMath::IsWithinInclusive(InAngle, -90.f - InTolerance, -45.f - InTolerance))
		{
			return EAngleDirection::Left;
		}
		else if (FMath::IsWithinInclusive(InAngle, -135.f - InTolerance, -90.f - InTolerance))
		{
			return EAngleDirection::BackwardLeft;
		}
	}

	// Return default result
	return InAngle > 0.f ? EAngleDirection::BackwardRF : EAngleDirection::BackwardLF;
}

EAngleDirection UVMM_Library::ConvertToAngleDirection(const EInputDirection& InInputDirection, const FRotator& InLastInputRotation)
{
	switch (InInputDirection)
	{
	case EInputDirection::Forward:
		return InLastInputRotation.Yaw >= 0.f ? EAngleDirection::ForwardRF : EAngleDirection::ForwardLF;

	case EInputDirection::ForwardLeft:
		return EAngleDirection::ForwardLeft;

	case EInputDirection::ForwardRight:
		return EAngleDirection::ForwardRight;

	case EInputDirection::Left:
		return EAngleDirection::Left;

	case EInputDirection::Right:
		return EAngleDirection::Right;

	case EInputDirection::Backward:
		return InLastInputRotation.Yaw >= 0.f ? EAngleDirection::BackwardRF : EAngleDirection::BackwardLF;

	case EInputDirection::BackwardLeft:
		return EAngleDirection::BackwardLeft;

	case EInputDirection::BackwardRight:
		return EAngleDirection::BackwardRight;

	case EInputDirection::MAX:
		return EAngleDirection::MAX;
	}

	// Return default result
	return EAngleDirection(InInputDirection);
}
#pragma endregion


#pragma region Cardinal Direction
FString UVMM_Library::GetCardinalDirectionString(const ECardinalDirection& InDirection)
{
	// Check the enum is valid
	if (UEnum* theEnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECardinalDirection"), true))
	{
		return theEnumPtr->GetNameStringByValue(int64(InDirection));
	}

	// Invalid result
	return TEXT("INVALID");
}

ECardinalDirection UVMM_Library::ConvertToCardinalDirection(const EAngleDirection InAngleDirection)
{
	switch (InAngleDirection)
	{
	case EAngleDirection::ForwardLF:
	case EAngleDirection::ForwardRF:
	case EAngleDirection::ForwardLeft:
	case EAngleDirection::ForwardRight:
		return ECardinalDirection::Forward;

	case EAngleDirection::Left:
		return ECardinalDirection::Left;

	case EAngleDirection::Right:
		return ECardinalDirection::Right;

	case EAngleDirection::BackwardLeft:
	case EAngleDirection::BackwardRight:
	case EAngleDirection::BackwardLF:
	case EAngleDirection::BackwardRF:
		return ECardinalDirection::Backward;
	}

	// Return default value
	return ECardinalDirection::MAX;
}
#pragma endregion

float UVMM_Library::GetRotationOffsetAngleValue(const ERotationMode InRotationMode, const EAngleDirection InAngleDirection)
{
	switch (InRotationMode)
	{
	case ERotationMode::Relaxed:
	case ERotationMode::UnEasy:
	case ERotationMode::Combat:
		switch (InAngleDirection)
		{
		case EAngleDirection::ForwardLeft:
			return -45.f;
		case EAngleDirection::ForwardRight:
			return 45.f;

		case EAngleDirection::BackwardLeft:
			return 45.f;
		case EAngleDirection::BackwardRight:
			return -45.f;
		}
		break;

	case ERotationMode::Velocity:
		return GetAngleDirectionValue(InAngleDirection);
	}

	// Return default value
	return 0.f;
}