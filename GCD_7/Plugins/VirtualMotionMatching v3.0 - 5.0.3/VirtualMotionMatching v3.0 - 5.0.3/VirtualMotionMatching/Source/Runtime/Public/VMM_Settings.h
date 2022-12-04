// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/SoftObjectPath.h"
#include "VMM_Types.h"
#include "VMM_Settings.generated.h"

class UDataTable;
class UVMM_InputMappingContext;
struct FSoftObjectPath;

#define GET_VMM_CONFIG_VAR(a) (GetDefault<UVMM_Settings>()->a)

/**
 *	Class for importing VirtualMotionMatching directly from a config file.
 */
UCLASS(config = Game, defaultconfig, notplaceable)
class VIRTUALMOTIONMATCHING_API UVMM_Settings : public UObject
{
	GENERATED_UCLASS_BODY()

#pragma region Config

public:

	/** Relative path to the ini file that is backing this list */
	UPROPERTY()
	FString ConfigFileName;

#if WITH_EDITORONLY_DATA
	/** If true, we will automatically copy the name of the Data Asset to DataName */
	UPROPERTY(config, EditAnywhere, Category = Editor)
	uint8 bRegisterDataName : 1;

    /** Flag register montage slots style */
	UPROPERTY(config, EditAnywhere, Category = Editor)
	uint8 bRegisterMontageSlotStyle : 1;
#endif // WITH_EDITORONLY_DATA

public:

	/** Save config data */
	void ResponseSaveConfig();

#pragma endregion


#pragma region Data

protected:

	/*
	 * Collect all animation skill data asset so that you can quickly get the specified data according to the index
	 * Not displayed in the editor, because it may cause lag
	 */
	UPROPERTY(config, VisibleAnywhere, Category = Data, meta = (MetaClass = "VMM_LocomotionData"))
	TArray<FSoftObjectPath> AnimSkillDataAssets;

	/*
	 * Collect all character attribute data table so that you can quickly get the specified data according to the index
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = Data, meta = (MetaClass = "DataTable"))
	TArray<FSoftObjectPath> CharacterAttributeTables;

public:

	/** Return desired data asset from sort index */
	const UVMM_LocomotionData* GetDataAsset(const int32 InDataIndex) const;

	/** Return desired params from sort index */
	const FLocomotionParams* GetParams(const FVirtualSkillParamsGroup& InSkillParamsGroup) const;

	/** Return desired character attribute data table from sort index */
	const FSoftObjectPath* GetCharacterAttributeTable(const uint16& InDataIndex) const;

#if WITH_EDITOR
	/** Collect all animation skill data assets, sort and cache them in virtual data. */
	void CollectSkillDataAssets();
#endif

#pragma endregion


#pragma region Curve

protected:

	/** Animation curves name reference map */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Curve")
	TMap<EMotionCurveType, FName> AnimCurvesNameMap;

public:

	/** Return the desired motion curve type name */
	FORCEINLINE const FName* GetMotionCurveName(const EMotionCurveType& InCurveType) const { return AnimCurvesNameMap.Find(InCurveType); }

#pragma endregion


#pragma region Trajectory

public:

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	int32 TrajectoryCount;

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	float TrajectoryTime;

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	float TrajectoryArrowSize;

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	float TrajectoryArrowThickness;

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	FLinearColor TrajectoryArrowColor;

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	float TrajectorySphereRadius;

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	int32 TrajectorySphereSegments;

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	float TrajectorySphereThickness;

	UPROPERTY(config, EditDefaultsOnly, Category = "Trajectory")
	FLinearColor TrajectorySpehreColor;

public:

	void TrajectoryAgentTransform(UObject* InInstance, const FTransform& InAgentTransform, const FTransform* InLastAgentTransform);

#pragma endregion

};
