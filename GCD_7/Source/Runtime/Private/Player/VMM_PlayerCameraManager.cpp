// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/VMM_PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/VMM_AnimInstance.h"
#include "Engine/Engine.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

static const FName NAME_Fixed = FName(TEXT("Fixed"));
static const FName NAME_ThirdPerson = FName(TEXT("ThirdPerson"));
static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
static const FName NAME_FreeCam_Default = FName(TEXT("FreeCam_Default"));
static const FName NAME_FirstPerson = FName(TEXT("FirstPerson"));
static const FName NAME_RightEye = FName(TEXT("RightEye_Socket"));

TAutoConsoleVariable<int32> CVarVirtualCamera_Debug(TEXT("a.VirtualMotionMatching.DebugCamera"), 0, TEXT("Debug virtual motion matching player camera data. "));

/*-----------------------------------------------------------------------------
	FCameraSettings implementation.
-----------------------------------------------------------------------------*/

void FCameraSettings::Lerp(const FCameraSettings& InLastSettings, const FCameraSettings& InDesiredSettings, const float& InAlpha)
{
	SocketOffset = UKismetMathLibrary::VLerp(InLastSettings.SocketOffset, InDesiredSettings.SocketOffset, InAlpha);
	SocketRotation = UKismetMathLibrary::RLerp(InLastSettings.SocketRotation, InDesiredSettings.SocketRotation, InAlpha, true);
	FOV = UKismetMathLibrary::Lerp(InLastSettings.FOV, InDesiredSettings.FOV, InAlpha);
	ArmLength = UKismetMathLibrary::Lerp(InLastSettings.ArmLength, InDesiredSettings.ArmLength, InAlpha);
	LagSpeed = UKismetMathLibrary::Lerp(InLastSettings.LagSpeed, InDesiredSettings.LagSpeed, InAlpha);
}

/*-----------------------------------------------------------------------------
	AVMM_PlayerCameraManager implementation.
-----------------------------------------------------------------------------*/

AVMM_PlayerCameraManager::AVMM_PlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LastDesiredLocation(ForceInitToZero)
	, CameraLagMaxTimeStep(1.f / 60.f)
	, ToggleSubtractViewDistance(50.f)
	, CameraArmLengthScale(1.f)
	, CameraArmLengthScaleRange(FVector2D(0.f, 1.5f))
	, CameraAdditiveArmLength(0.f)
	, InterpingCurvePosition(0.f)
	, InterpingCurveMaxPosition(0.f)
{
	ViewMode = ECameraViewMode::ThirdPerson;
	TargetViewMode = ViewMode;
	CameraStyle = NAME_ThirdPerson;
}

void AVMM_PlayerCameraManager::InitializeFor(class APlayerController* InPC)
{
	Super::InitializeFor(InPC);

	// Initialize default camera settings
	SetCameraSettings(DefaultCameraSettingTag, INDEX_NONE, false);
}

void AVMM_PlayerCameraManager::SetViewTarget(class AActor* InViewTarget, FViewTargetTransitionParams InTransitionParams)
{
	Super::SetViewTarget(InViewTarget, InTransitionParams);

	// Initialize default reference
	if (ACharacter* theViewCharacter = Cast<ACharacter>(InViewTarget))
	{
		LastDesiredLocation = InViewTarget->GetActorLocation();
		PCOwnerAnimInstance = Cast<UVMM_AnimInstance>(theViewCharacter->GetMesh() ? theViewCharacter->GetMesh()->GetAnimInstance() : nullptr);
	}
}

void AVMM_PlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// Don't update outgoing view target during an interpolation 
	if ((PendingViewTarget.Target != NULL) && BlendParams.bLockOutgoing && OutVT.Equal(ViewTarget))
	{
		return;
	}

	// Update camera settings
	OnUpdateInterpCameraSettings(DeltaTime);

	// Store previous POV, in case we need it later
	FMinimalViewInfo OrigPOV = OutVT.POV;

	//@TODO: CAMERA: Should probably reset the view target POV fully here
	OutVT.POV.FOV = DefaultFOV;
	OutVT.POV.OrthoWidth = DefaultOrthoWidth;
	OutVT.POV.AspectRatio = DefaultAspectRatio;
	OutVT.POV.bConstrainAspectRatio = bDefaultConstrainAspectRatio;
	OutVT.POV.bUseFieldOfViewForLOD = true;
	OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
	OutVT.POV.PostProcessSettings.SetBaseValues();
	OutVT.POV.PostProcessBlendWeight = 1.0f;

	// Initialize default out data
	bool bDoNotApplyModifiers = false;
	OutVT.Target->GetActorEyesViewPoint(OutVT.POV.Location, OutVT.POV.Rotation);
	OutVT.POV.Rotation = PCOwner->GetControlRotation();

	// Check the camera socket rotation offset is valid
	if (CameraSettings.SocketRotation != FRotator::ZeroRotator)
	{
		OutVT.POV.Rotation -= CameraSettings.SocketRotation;
		OutVT.POV.Rotation.Normalize();
	}

	// Default camera actor
	if (ACameraActor* theCamActor = Cast<ACameraActor>(OutVT.Target))
	{
		// Viewing through a camera actor.
		if (theCamActor->GetCameraComponent())
		{
			theCamActor->GetCameraComponent()->GetCameraView(DeltaTime, OutVT.POV);
		}
	}
	else
	{
		// Fixed Camera
		if (CameraStyle == NAME_Fixed)
		{
			// Do not update, keep previous camera position by restoring
			// Saved POV, in case CalcCamera changes it but still returns false
			OutVT.POV = OrigPOV;

			// Don't apply modifiers when using this debug camera mode
			bDoNotApplyModifiers = true;
		}
		else if (CameraStyle == NAME_FirstPerson) // First Person Camera
		{
			check(PCOwner);

			// Get the eye location from view target
			OutVT.POV.Location = GetEyeLocation(OutVT);

			// Cache the focal location
			LastDesiredLocation = PCOwner->GetFocalLocation();

			// Don't apply modifiers when using this debug camera mode
			bDoNotApplyModifiers = true;
		}
		else if (CameraStyle == NAME_ThirdPerson) // Third Person Camera
		{
			check(PCOwner);

			// Get the spring arm 'origin', the target we want to look at
			FVector theArmOrigin = PCOwner->GetFocalLocation();
		
			// We lag the target, not the actual camera position, so rotating the camera around does not have lag
			FVector theDesiredLocation = theArmOrigin;
			if (DeltaTime > CameraLagMaxTimeStep && CameraSettings.LagSpeed > 0.f)
			{
				const FVector ArmMovementStep = (theDesiredLocation - LastDesiredLocation) * (1.f / DeltaTime);
				FVector LerpTarget = LastDesiredLocation;

				float RemainingTime = DeltaTime;
				while (RemainingTime > KINDA_SMALL_NUMBER)
				{
					const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
					LerpTarget += ArmMovementStep * LerpAmount;
					RemainingTime -= LerpAmount;

					theDesiredLocation = FMath::VInterpTo(LastDesiredLocation, LerpTarget, LerpAmount, CameraSettings.LagSpeed);
					LastDesiredLocation = theDesiredLocation;
				}
			}
			else
			{
				theDesiredLocation = FMath::VInterpTo(LastDesiredLocation, theDesiredLocation, DeltaTime, CameraSettings.LagSpeed);
			}

			// Calculate the additive camera length
			float theDesiredAdditiveLength = 0.f;

			// Simple simulation of positive effects based on camera and character orientation
			if (OutVT.Target)
			{
				const FRotator theDeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(OutVT.POV.Rotation, OutVT.Target->GetActorRotation());
				if (FMath::Abs(theDeltaRotation.Yaw) >= 120.f)
				{
					theDesiredAdditiveLength = 70.f;
				}
			}

			// Smooth the additive camera length
			CameraAdditiveArmLength = FMath::FInterpTo(CameraAdditiveArmLength, theDesiredAdditiveLength, DeltaTime, 1.f);

			// Cache previous location
			LastDesiredLocation = theDesiredLocation;

			// Now offset camera position back along our rotation
			theDesiredLocation -= OutVT.POV.Rotation.Vector() * (CameraSettings.ArmLength * CameraArmLengthScale + CameraAdditiveArmLength);
			
			// Add socket offset in local space
			theDesiredLocation += FRotationMatrix(OutVT.POV.Rotation).TransformVector(CameraSettings.SocketOffset);

#if ENABLE_DRAW_DEBUG
			if (CVarVirtualCamera_Debug.GetValueOnAnyThread() > 0)
			{
				DrawDebugSphere(GetWorld(), theArmOrigin, 5.f, 8, FColor::Green);
				DrawDebugSphere(GetWorld(), theDesiredLocation, 5.f, 8, FColor::Yellow);

				const FVector ToOrigin = theArmOrigin - theDesiredLocation;
				DrawDebugDirectionalArrow(GetWorld(), theDesiredLocation, theDesiredLocation + ToOrigin * 0.5f, 7.5f, FColor::Green);
				DrawDebugDirectionalArrow(GetWorld(), theDesiredLocation + ToOrigin * 0.5f, theArmOrigin, 7.5f, FColor::Green);
			}
#endif // ENABLE_DRAW_DEBUG

			// Calculate the camera collision
			FHitResult theResult;
			FCollisionQueryParams BoxParams(SCENE_QUERY_STAT(FreeCam), false, this);
			BoxParams.AddIgnoredActor(OutVT.Target);
			GetWorld()->SweepSingleByChannel(theResult, theArmOrigin, theDesiredLocation, FQuat::Identity, ECC_Camera, FCollisionShape::MakeBox(FVector(12.f)), BoxParams);
			OutVT.POV.Location = !theResult.bBlockingHit ? theDesiredLocation : theResult.Location;
		}
		else if (CameraStyle == NAME_FreeCam || CameraStyle == NAME_FreeCam_Default) // Free Camera
		{
			// Simple third person view implementation
			FVector Loc = OutVT.Target->GetActorLocation();
			FRotator Rotator = OutVT.Target->GetActorRotation();

			if (OutVT.Target == PCOwner)
			{
				Loc = PCOwner->GetFocalLocation();
			}

			if (CameraStyle == NAME_FreeCam || CameraStyle == NAME_FreeCam_Default)
			{
				Rotator = PCOwner->GetControlRotation();
			}

			FVector Pos = Loc + ViewTargetOffset + FRotationMatrix(Rotator).TransformVector(FreeCamOffset) - Rotator.Vector() * FreeCamDistance;
			FCollisionQueryParams BoxParams(SCENE_QUERY_STAT(FreeCam), false, this);
			BoxParams.AddIgnoredActor(OutVT.Target);
			FHitResult Result;

			GetWorld()->SweepSingleByChannel(Result, Loc, Pos, FQuat::Identity, ECC_Camera, FCollisionShape::MakeBox(FVector(12.f)), BoxParams);
			OutVT.POV.Location = !Result.bBlockingHit ? Pos : Result.Location;
			OutVT.POV.Rotation = Rotator;
		}
		else
		{
			UpdateViewTargetInternal(OutVT, DeltaTime);
		}
	}

	// Apply camera modifiers at the end (view shakes for example)
	if (!bDoNotApplyModifiers || bAlwaysApplyModifiers)
	{
		ApplyCameraModifiers(DeltaTime, OutVT.POV);
	}

	// Synchronize the actor with the view target results
	SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);

	// Update camera lens effects
	UpdateCameraLensEffects(OutVT);
}

FVector AVMM_PlayerCameraManager::GetEyeLocation(FTViewTarget& InViewTarget) const
{
	// Always check the animation instance is valid
	if (PCOwnerAnimInstance.IsValid())
	{
		return PCOwnerAnimInstance->GetEyeLocation();
	}

	// Check the default camera socket location
	if (ACharacter* theCharacter = Cast<ACharacter>(InViewTarget.Target))
	{
		return theCharacter->GetMesh() ? theCharacter->GetMesh()->GetSocketLocation(CameraSettings.SocketName) : InViewTarget.POV.Location;
	}

	// Return default result
	return InViewTarget.POV.Location;
}

#pragma region Camera Setting
void AVMM_PlayerCameraManager::OnUpdateInterpCameraSettings(const float& InDeltaTime)
{
	// Check the target camera setting index is valid
	if (!DesiredCameraSettingTag.IsSet())
	{
		return;
	}

	// Check the desired camera setting is valid
	const FCameraSettings* theDesiredCameraSetting = GetCameraSettings(DesiredCameraSettingTag.GetValue());
	if (theDesiredCameraSetting == nullptr)
	{
		DesiredCameraSettingTag.Reset();
		return;
	}

	// Get the camera curve reference
	const FRuntimeFloatCurve* theCameraCurve = DesiredCameraCurveIndex.IsSet() ? GetCameraCurve(DesiredCameraCurveIndex.GetValue()) : nullptr;
	const FRichCurve* theCameraRichCurve = theCameraCurve ? theCameraCurve->GetRichCurveConst() : nullptr;

	// Define use curve lerp or time lerp
	const bool bUseCurveLerp = theDesiredCameraSetting->bInterp && theCameraRichCurve != nullptr;

	// Calculate the lerp value
	float theLerpAlpha = bUseCurveLerp ? theCameraRichCurve->Eval(InterpingCurvePosition) : (InterpingCurveMaxPosition == 0.f ? 1.f : (InterpingCurvePosition / InterpingCurveMaxPosition));
	theLerpAlpha = FMath::Clamp(theLerpAlpha, 0.f, 1.f);

	// Lerp two settings data
	CameraSettings.Lerp(LastCameraSettings, *theDesiredCameraSetting, theLerpAlpha);

	// Check the view mode toggle state
	if (ShouldToggleViewMode(TargetViewMode))
	{
		TransferViewMode();
	}

	// If the curve position is invalid, we should stop lerp it.
	if (InterpingCurvePosition >= InterpingCurveMaxPosition)
	{
		DesiredCameraSettingTag.Reset();
		return;
	}

	// Accumulate the curve position
	InterpingCurvePosition += InDeltaTime;
}

bool AVMM_PlayerCameraManager::SetCameraSettings(const FGameplayTag& InGameplayTag, int32 InCurveIndex, bool Interp)
{
	// Check the desired camera setting is valid
	const FCameraSettings* theDesiredCameraSetting = GetCameraSettings(InGameplayTag);
	if (theDesiredCameraSetting == nullptr)
	{
		return false;
	}
	
	// If is not smooth, we should only transfer data
	if (Interp == false)
	{
		CameraSettings = *theDesiredCameraSetting;
		return true;
	}

	// Clamp the curve index
	InCurveIndex = FMath::Clamp(InCurveIndex, 0, CameraCurves.Num());

	// Cache the desired camera setting index
	DesiredCameraCurveIndex = InCurveIndex;
	DesiredCameraSettingTag = InGameplayTag;

	// Check the camera curve is valid
	const FRuntimeFloatCurve* theCameraCurve = GetCameraCurve(InCurveIndex);
	if (theCameraCurve == nullptr || !theCameraCurve->GetRichCurveConst() || !theCameraCurve->GetRichCurveConst()->HasAnyData())
	{
		InterpingCurveMaxPosition = theDesiredCameraSetting->InterpingTime;
	}
	else
	{
		InterpingCurveMaxPosition = theCameraCurve->GetRichCurveConst()->GetLastKey().Time;
	}

	// Cache current camera settings as lerp initialize data
	LastCameraSettings = CameraSettings;

	// Clear runtime data
	InterpingCurvePosition = 0.f;

	// Transfer not lerp data
	CameraSettings.SocketName = theDesiredCameraSetting->SocketName;
	CameraSettings.bInterp = Interp ? theDesiredCameraSetting->bInterp : false;
	CameraSettings.InterpingTime = theDesiredCameraSetting->InterpingTime;

#if WITH_EDITORONLY_DATA
	CameraSettings.Description = theDesiredCameraSetting->Description;
#endif // WITH_EDITORONLY_DATA

	// Success
	return true;
}

void AVMM_PlayerCameraManager::SetCameraArmLengthScale(const float InValue)
{
	CameraArmLengthScale += InValue;
	CameraArmLengthScale = FMath::Clamp(CameraArmLengthScale, CameraArmLengthScaleRange.X, CameraArmLengthScaleRange.Y);
}

const FRuntimeFloatCurve* AVMM_PlayerCameraManager::GetCameraCurve(const int32& InElementIndex) const
{
	return CameraCurves.IsValidIndex(InElementIndex) ? &CameraCurves[InElementIndex] : nullptr;
}

const FCameraSettings* AVMM_PlayerCameraManager::GetCameraSettings(const FGameplayTag& InGameplayTag) const
{
	return CameraSettingsMap.Find(InGameplayTag);
}
#pragma endregion


#pragma region View Mode
void AVMM_PlayerCameraManager::TransferViewMode()
{
	// Check the target view mode is valid
	if (ViewMode == TargetViewMode)
	{
		return;
	}

	// Cache the view mode
	ViewMode = TargetViewMode;

	// Toggle camera style
	switch (ViewMode)
	{
	case ECameraViewMode::FirstPerson:
		CameraStyle = NAME_FirstPerson;
		break;

	case ECameraViewMode::ThirdPerson:
		CameraStyle = NAME_ThirdPerson;
		break;

	case ECameraViewMode::TopDownPerson:
		CameraStyle = NAME_ThirdPerson;
		break;
	}

	// Notify animation instance
	if (PCOwnerAnimInstance.IsValid())
	{
		PCOwnerAnimInstance->SetFirstPersonMode(ViewMode == ECameraViewMode::FirstPerson);
	}
}

bool AVMM_PlayerCameraManager::CanToggleViewMode(const ECameraViewMode& InViewMode)
{
	// Handle target view mode state
	switch (TargetViewMode)
	{
	case ECameraViewMode::FirstPerson:
		return CameraSettings.ArmLength <= ToggleSubtractViewDistance;

	case ECameraViewMode::ThirdPerson:
		return true;

	case ECameraViewMode::TopDownPerson:
		return true;
	}

	// INVALID
	return true;
}

bool AVMM_PlayerCameraManager::ShouldToggleViewMode(const ECameraViewMode& InViewMode)
{
	return ViewMode != InViewMode && CanToggleViewMode(InViewMode);
}

void AVMM_PlayerCameraManager::SetViewMode(const ECameraViewMode& InViewMode, const FGameplayTag* InGameplayTag)
{
	// Check the view mode is valid
	if (TargetViewMode == InViewMode)
	{
		return;
	}

	// Cache the target data
	TargetViewMode = InViewMode;
	TargetCameraTag = InGameplayTag == nullptr ? DefaultCameraSettingTag : *InGameplayTag;

	// Set new camera settings
	if (SetCameraSettings(TargetCameraTag) == false)
	{
		// If haven't smooth event, we should apply view mode now.
		TransferViewMode();
	}
	else if (ViewMode == ECameraViewMode::FirstPerson)
	{
		// If last view mode is first person, we should initialize start length
		LastCameraSettings.ArmLength = ToggleSubtractViewDistance;
	}
}
#pragma endregion