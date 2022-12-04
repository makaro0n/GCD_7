// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VMM_Types.h"
#include "DataRegistryId.h"
#include "VMM_Library.generated.h"


/**
 *
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_Library : public UObject
{
	GENERATED_BODY()

public:

#pragma region Network

	/** Return role name from actor */
	static FString GetRoleName(const AActor* InActor);

	/** Return authority role state from actor */
	static bool IsAuthority(const AActor* InActor);

	/** Return autonomous proxy role state from actor */
	static bool IsAutonomousProxy(const AActor* InActor);

	/** Return simulate proxy role state from actor */
	static bool IsSimulatedProxy(const AActor* InActor);

#pragma endregion

	/**
	 * Simple wait time.
	 * @param	InDeltaSeconds	  Delta time.
	 * @param	InValue	 Accumulate time value.
	 * @param	InWaitTime	 Wait max time
	 * @return  wait state
	 */
	UFUNCTION(BlueprintCallable, Category = "VMM| Library")
	static bool WaitTime(const float& InDeltaSeconds, UPARAM(ref) float &InValue, const float InWaitTime);

#pragma region Data Registry

	/** Return the registry data from data registry id */
	static bool GetRegistryData(const FDataRegistryId& InRegistryId, void* OutData);

#pragma endregion


#pragma region Input Direction

	/**
	 * Convert input direction float value to input direction type
	 * @param	InAngle	  Target angle value
	 * @param	InTolerance	  Tolerance for each angle
	 * @return  Input direction type
	 */
	UFUNCTION(BlueprintPure, Category = "VMM| Library")
	static EInputDirection GetInputDirectionType(const float InAngle, const float InTolerance = 22.5f);

#pragma endregion


#pragma region Angle Direction
	/**
	 * Convert angle direction type to angle float value
	 * @param	InAngleDirection	  Target angle direction
	 * @return  Angle direction float value
	 */
	UFUNCTION(BlueprintPure, Category = "VMM| Library")
	static float GetAngleDirectionValue(const EAngleDirection InAngleDirection);

	/** Return the angle direction type string */
	UFUNCTION(BlueprintPure, Category = "VMM| Library")
	static FString GetAngleDirectionString(const EAngleDirection& InDirection);

	/**
	 * Convert angle direction float value to angle direction type
	 * @param	InAngle	  Target angle value
	 * @param	InTolerance	  Tolerance for each angle
	 * @return  Angle direction type
	 */
	UFUNCTION(BlueprintPure, Category = "VMM| Library")
	static EAngleDirection GetAngleDirectionType(const float InAngle, const float InTolerance = 22.5f);

	/**
	 * Convert input direction to angle direction
	 * @param	InInputDirection	  Input direction
	 * @param	InLastInputRotation	  Last input rotation
	 * @return  Cardinal direction type
	 */
	UFUNCTION(BlueprintPure, Category = "VMM| Library")
	static EAngleDirection ConvertToAngleDirection(const EInputDirection& InInputDirection, const FRotator& InLastInputRotation);

#pragma endregion


#pragma region Cardinal Direction

	/** Return the cardinal direction type string */
	UFUNCTION(BlueprintPure, Category = "VMM| Library")
	static FString GetCardinalDirectionString(const ECardinalDirection& InDirection);

	/**
	 * Convert angle direction to cardinal direction
	 * @param	InAngleDirection	  angle direction
	 * @return  Cardinal direction type
	 */
	UFUNCTION(BlueprintPure, Category = "VMM| Library")
	static ECardinalDirection ConvertToCardinalDirection(const EAngleDirection InAngleDirection);

#pragma endregion

	/**
	 * Convert cardinal direction type to rotation additive angle value
	 * @param	InRotationMode	  Movement rotation mode
	 * @param	InAngleDirection	  Target angle direction
	 * @return  Rotation additive angle value
	 */
	UFUNCTION(BlueprintPure, Category = "VMM| Library")
	static float GetRotationOffsetAngleValue(const ERotationMode InRotationMode, const EAngleDirection InAngleDirection);
};