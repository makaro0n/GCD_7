// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Camera/PlayerCameraManager.h"
#include "VMM_Types.h"
#include "VMM_PlayerCameraManager.generated.h"

class UVMM_AnimInstance;
struct FRuntimeFloatCurve;

/** Struct of camera settings data */
USTRUCT(BlueprintType)
struct FCameraSettings
{
	GENERATED_USTRUCT_BODY()

#if WITH_EDITORONLY_DATA
	/** Description of this data, editor only */
	UPROPERTY(EditDefaultsOnly, Category = Editor)
	FText Description;
#endif // WITH_EDITORONLY_DATA

	/** FOV of camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	float FOV;

	/** Arm length of camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	float ArmLength;

	/** Lag speed of camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	float LagSpeed;

	/** Mesh socket name of camera, if is valid, we always use socket location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Socket)
	FName SocketName;

	/** Local location offset data of camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Socket)
	FVector SocketOffset;

	/** Local rotation offset data of camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Socket)
	FRotator SocketRotation;

	/** Flag the camera use smooth condition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interp)
	uint8 bInterp : 1;

	/** Interp time of camera setting changed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interp)
	float InterpingTime;

	/** curve reference index of global curves */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interp)
	int32 CurveReferenceIndex;

public:

	/** Lerp two camera settings data to self */
	void Lerp(const FCameraSettings& InLastSettings, const FCameraSettings& InDesiredSettings, const float& InAlpha);

	FCameraSettings()
		: FOV(90.f)
		, ArmLength(300.f)
		, LagSpeed(10.f)
		, SocketName(NAME_None)
		, SocketOffset(FVector(5.f, 14.f, 0.f))
		, SocketRotation(FRotator::ZeroRotator)
		, bInterp(true)
		, InterpingTime(0.5f)
		, CurveReferenceIndex(0)
	{}
};

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API AVMM_PlayerCameraManager : public APlayerCameraManager
{
	GENERATED_UCLASS_BODY()

private:

	/** Animation instance that owns this Camera actor */
	TWeakObjectPtr<UVMM_AnimInstance> PCOwnerAnimInstance;

public:
	virtual void InitializeFor(class APlayerController* InPC) override;
	virtual void SetViewTarget(class AActor* InViewTarget, FViewTargetTransitionParams InTransitionParams = FViewTargetTransitionParams()) override;
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

protected:

	/** Return eye view location from view target */
	virtual FVector GetEyeLocation(FTViewTarget& InViewTarget) const;

#pragma region Camera Setting

protected:

	/** Cached last desired location */
	FVector LastDesiredLocation;

	/** Max time step of camera */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Setting")
	float CameraLagMaxTimeStep;

	/** FPP/TPP ignore distance of camera */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Setting")
	float ToggleSubtractViewDistance;

	/** Arm length scale size of camera */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Setting")
	float CameraArmLengthScale;

	/** Arm length scale range of camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setting")
	FVector2D CameraArmLengthScaleRange;

	/** Initialize camera settings as gameplay tag */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Setting")
	FGameplayTag DefaultCameraSettingTag;

	/** Camera smooth curves data */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Setting")
	TArray<FRuntimeFloatCurve> CameraCurves;

	/** Camera settings as gameplay tag */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Setting")
	TMap<FGameplayTag, FCameraSettings> CameraSettingsMap;

protected:

	/** Cached additive arm length of camera */
	float CameraAdditiveArmLength;

	/** Cached smooth curve position */
	float InterpingCurvePosition;

	/** Cached smooth curve max position */
	float InterpingCurveMaxPosition;

	/** Cached current camera settings data */
	FCameraSettings CameraSettings;

	/** Cached last camera settings data, is lerp initialize data */
	FCameraSettings LastCameraSettings;

	/** Cached desired camera curve index */
	TOptional<int32> DesiredCameraCurveIndex;

	/** Cached desired camera setting tag */
	TOptional<FGameplayTag> DesiredCameraSettingTag;

	/** Response update smooth camera settings data */
	virtual void OnUpdateInterpCameraSettings(const float& InDeltaTime);

public:

	/** Set desired camera settings */
	virtual bool SetCameraSettings(const FGameplayTag& InGameplayTag, int32 InCurveIndex = INDEX_NONE, bool Interp = true);
	
	/** Set desired camera arm length scale */
	virtual void SetCameraArmLengthScale(const float InValue);

public:

	/** Return the desired camera curve from index */
	virtual const FRuntimeFloatCurve* GetCameraCurve(const int32& InElementIndex) const;

	/** Return the desired camera setting from index */
	virtual const FCameraSettings* GetCameraSettings(const FGameplayTag& InGameplayTag) const;

#pragma endregion


#pragma region View Mode

protected:

	/** Current active view mode of camera */
	ECameraViewMode ViewMode;

	/** Desired view mode of camera, wait event finished */
	ECameraViewMode TargetViewMode;

	/** Desired gameplay tag of camera, @See CameraSettings */
	FGameplayTag TargetCameraTag;

protected:

	/** Transfer target view mode to current view mode */
	void TransferViewMode();

	/** Return toggle view mode condition */
	bool CanToggleViewMode(const ECameraViewMode& InViewMode);

	/** Return should toggle view mode, we will check current and target state */
	bool ShouldToggleViewMode(const ECameraViewMode& InViewMode);

public:

	/** Set new view mode of camera */
	void SetViewMode(const ECameraViewMode& InViewMode, const FGameplayTag* InGameplayTag);

public:

	/** Return current active view mode of camera */
	FORCEINLINE const ECameraViewMode& GetViewMode() { return ViewMode; }

#pragma endregion
};
