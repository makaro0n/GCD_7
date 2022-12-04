// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VMM_Types.h"
#include "VMM_MontageLibrary.generated.h"

class UAnimMontage;
class UAnimSequence;
class UAnimInstance;
struct FAnimMontageInstance;

/**
 *
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_MontageLibrary : public UObject
{
	GENERATED_BODY()

	/*-----------------------------------------------------------------------------
		Montage Library implementation.
	-----------------------------------------------------------------------------*/

public:

	/** Get MontageInstance from AnimInstance */
	static FAnimMontageInstance* GetMontageInstance(UAnimInstance* InAnimInstance, UAnimMontage* InMontage);

	/** Get ActiveMontageInstance from AnimInstance */
	static FAnimMontageInstance* GetActiveInstanceForMontage(UAnimInstance* InAnimInstance, UAnimMontage* InMontage);

	/** Set animation asset root motion condition */
	UFUNCTION(BlueprintCallable, Category = "VMM| RootMotion")
	static void SetRootMotion(UAnimSequence* InAnimSequence, bool InEnableRootMotion);
	
	/** Return active montage playing state from AnimInstance */
	UFUNCTION(BlueprintPure, Category = "VMM| Montage")
	static bool GetMontageIsPlaying(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, const bool InActiveMontage = true);

	/** Return active montage play position from AnimInstance */
	UFUNCTION(BlueprintPure, Category = "VMM| Montage")
	static float GetMontagePlayPosition(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, const bool InActiveMontage = true);

	/** Return active montage play rate and play position from AnimInstance */
	UFUNCTION(BlueprintPure, Category = "VMM| Montage")
	static bool GetMontagePlayRatePosition(UAnimInstance* InAnimInstance, UAnimMontage* InMontage
			, float& OutPlayRate, float& OutPlayPosition, const bool InActiveMontage = true);
	
	/** Return active montage play section and next section from AnimInstance */
	UFUNCTION(BlueprintPure, Category = "VMM| Montage")
	static bool GetMontagePlaySection(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, FName& OutSectionName, const bool InActiveMontage = true);
	
	/** Return active montage play section and next section from AnimInstance */
	UFUNCTION(BlueprintPure, Category = "VMM| Montage")
	static bool GetMontagePlayNextSection(UAnimInstance* InAnimInstance, UAnimMontage* InMontage
			, FName& OutSectionName, FName& OutNextSectionName, const bool InActiveMontage = true);

	/** Return montage blend weight from AnimInstance */
	UFUNCTION(BlueprintPure, Category = "VMM| Montage")
	static float GetMontageBlendWeight(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, const bool InActiveMontage = true);

	/** Return active montages playing state from AnimInstance */
	UFUNCTION(BlueprintPure, Category = "VMM| Montage")
	static bool HasPlayingMontages(UAnimInstance* InAnimInstance, TArray<UAnimMontage*> InMontages, const bool InActiveMontage, int32& OutCount);

	/** Disable montage root motion */
	UFUNCTION(BlueprintCallable, Category = "VMM| Montage")
	static void PushDisableRootMotion(UAnimInstance* InAnimInstance, UAnimMontage* InMontage);

	/** Stop montage, return montage stop state from AnimInstance */
	UFUNCTION(BlueprintCallable, Category = "VMM| Montage")
	static bool MontageStop(UAnimInstance* InAnimInstance, UAnimMontage* InMontage, float InBlendOutTime = -1.f, bool bInterrupt = true);

	/** Set montage blend time */
	UFUNCTION(BlueprintCallable, Category = "VMM| Montage")
	static void SetMontageBlendTime(UAnimMontage* InMontage, const bool InBlendOut, const float InBlendTime);

	/** Matching two montage blend time, get last montage blend remaining time to next montage blend in time */
	UFUNCTION(BlueprintCallable, Category = "VMM| Montage")
	static void MatchingMontageBlendRemainingTime(UAnimInstance* InAnimInstance, UAnimMontage* InLastMontage, UAnimMontage* InNextMontage);

	/** Utility function to create dynamic montage from AnimSequence */
	static UAnimMontage* CreateSlotAnimationAsDynamicMontage(UObject* InWorldContextObject, const FAnimSkillParams* InSkillParams);

	/** Debug current all montage instances data */
	UFUNCTION(BlueprintCallable, Category = "VMM| Montage")
	static void DebugMontageInstances(UAnimInstance* InAnimInstance);

#pragma region Montage Event

	/** Clear montage bind event from AnimInstance */
	static void ClearMontageBindEvent(UAnimInstance* InAnimInstance, UAnimMontage* InMontage);

	/** Clear montage bind out event from AnimInstance */
	static void ClearMontageBindOutEvent(UAnimInstance* InAnimInstance, UAnimMontage* InMontage);

	/** Clear montage bind ended event from AnimInstance */
	static void ClearMontageBindEndedEvent(UAnimInstance* InAnimInstance, UAnimMontage* InMontage);

#pragma endregion
};