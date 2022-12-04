// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "GameFramework/VMM_CharacterMovementComponent.h"
#include "GameFramework/VMM_Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Librarys/VMM_Library.h"
#include <GameFramework/GameNetworkManager.h>

UVMM_CharacterMovementComponent::UVMM_CharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CharacterRotation(ForceInitToZero)
	, IgnoreUpdatePhysicsCharacterRotation(0)
	, RotationScaleRange(FVector2D(1.f))
	, bHasMovementInput(false)
	, DirectionWeight(FVector(1.f))
	, MovementRotation(ForceInitToZero)
	, LastMovementRotation(ForceInitToZero)
	, MovementDirection(EAngleDirection::MAX)
	, LastMovementDirection(EAngleDirection::MAX)
	, MovementCardinalDirection(ECardinalDirection::MAX)
	, LastMovementCardinalDirection(ECardinalDirection::MAX)
{
	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanFly = true;

#if ENGINE_MAJOR_VERSION > 4
	SetCrouchedHalfHeight(60.f);
#else
	CrouchedHalfHeight = 60.f;
#endif // ENGINE_MAJOR_VERSION > 4
	NetworkBlendRootMotionClientCorrectionDistance = 200.f;

	bCanWalkOffLedgesWhenCrouching = true;

	RotationRate.Yaw = 5.f;
	bUseControllerDesiredRotation = true;
	bAllowPhysicsRotationDuringAnimRootMotion = true;
}

#pragma region Owner
TAutoConsoleVariable<int32> CVarVirtualMotionMatching_DebugMovementDirection(TEXT("a.VirtualMotionMatching.DebugMovementDirection"), 0, TEXT("Debug virtual motion matching movement direction data. "));
void UVMM_CharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Super
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if ENABLE_DRAW_DEBUG
	if (CVarVirtualMotionMatching_DebugMovementDirection.GetValueOnAnyThread() > 0 && CharacterOwner)
	{
		FString theDebugString = FString::Printf(TEXT("UVMM_CharacterMovementComponent::TickComponent:: LockRotation: %s, LockMovement: %s")
			, *UKismetStringLibrary::Conv_BoolToString(IsUpdateRotationIgnored())
			, CharacterOwner->GetController() ? *UKismetStringLibrary::Conv_BoolToString(CharacterOwner->GetController()->IsMoveInputIgnored()) : false);
		UKismetSystemLibrary::PrintString(this, theDebugString, true, true, FLinearColor::Red, 0.f);

		FVector theActorLocation = UpdatedComponent->GetComponentLocation();
		FRotator theLookingRotation(0.f, CharacterOwner->GetControlRotation().Yaw, 0.f);
		UKismetSystemLibrary::DrawDebugArrow(this, theActorLocation, theActorLocation + theLookingRotation.Vector() * 100.f, 100.f, FLinearColor::Blue, 0.f, 2.f);
		theActorLocation.Z += 30.f;
		UKismetSystemLibrary::DrawDebugArrow(this, theActorLocation, theActorLocation + CharacterRotation.Vector() * 100.f, 100.f, FLinearColor::Green, 0.f, 2.f);
		theActorLocation.Z += 30.f;
		UKismetSystemLibrary::DrawDebugArrow(this, theActorLocation, theActorLocation + CharacterOwner->GetActorRotation().Vector() * 100.f, 100.f, FLinearColor::Red, 0.f, 2.f);
		theActorLocation.Z += 30.f;
		FRotator theDesiredRotation = theLookingRotation;
		CalculateDesiredRotation(theDesiredRotation);
		UKismetSystemLibrary::DrawDebugArrow(this, theActorLocation, theActorLocation + theDesiredRotation.Vector() * 100.f, 100.f, FLinearColor::Yellow, 0.f, 2.f);
	}
#endif // ENABLE_DRAW_DEBUG
}

void UVMM_CharacterMovementComponent::PostLoad()
{
	Super::PostLoad();

	// Initialize character owner rotation
	CharacterRotation = UpdatedComponent ? UpdatedComponent->GetComponentQuat() : FQuat::Identity;
}

void UVMM_CharacterMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	Super::SetUpdatedComponent(NewUpdatedComponent);

	// Initialize the animation skill character owner
	VMM_CharacterOwner = Cast<AVMM_Character>(CharacterOwner);

	// Initialize character owner rotation
	CharacterRotation = NewUpdatedComponent ? NewUpdatedComponent->GetComponentQuat() : FQuat::Identity;
}

void UVMM_CharacterMovementComponent::PerformMovement(float InDeltaTime)
{
	Super::PerformMovement(InDeltaTime);

	// Check the rotation updated state
	if (PhysicsRotationFrame == GFrameCounter)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION < 5
	if (CharacterOwner->IsMatineeControlled())
	{
		return;
	}
#endif // ENGINE_MAJOR_VERSION < 5

	// Try physics desired rotation - RootMotion
	bCanPerformRotation = CanPerformRotation();

	// If don't perform rotation, we should reset the value.
	if (bCanPerformRotation.GetValue() == false)
	{
		bCanPerformRotation.Reset();
		return;
	}

	// We should reset last simulate motion rotation?
	if (LastRootMotionRotation.IsSet())
	{
		MoveUpdatedComponent(FVector::ZeroVector, LastRootMotionRotation->Inverse() * UpdatedComponent->GetComponentQuat(), true);
		LastRootMotionRotation.Reset();
	}

	// Physics rotation again
	PhysicsRotation(InDeltaTime);
}

void UVMM_CharacterMovementComponent::SmoothCorrection(const FVector& OldLocation, const FQuat& OldRotation, const FVector& NewLocation, const FQuat& NewRotation)
{
	// Always copy the new character rotation - VRM
	// Root Motion don't copy it.
	CharacterRotation = NewRotation;
	//UE_LOG(LogVirtualMotionMatching, Warning, TEXT("UVMM_CharacterMovementComponent::SmoothCorrection, Rot: %s."), *CharacterRotation.ToString());

	// Super
	Super::SmoothCorrection(OldLocation, OldRotation, NewLocation, NewRotation);
}

bool UVMM_CharacterMovementComponent::ServerExceedsAllowablePositionError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	// Check for disagreement in movement mode
	const uint8 CurrentPackedMovementMode = PackNetworkMovementMode();
	if (CurrentPackedMovementMode != ClientMovementMode)
	{
		// Consider this a major correction, see SendClientAdjustment()
		bNetworkLargeClientCorrection = true;
		return true;
	}

	const FVector LocDiff = UpdatedComponent->GetComponentLocation() - ClientWorldLocation;
	const AGameNetworkManager* GameNetworkManager = (const AGameNetworkManager*)(AGameNetworkManager::StaticClass()->GetDefaultObject());
	if (GameNetworkManager->ExceedsAllowablePositionError(LocDiff))
	{
		// @VIRTUALGAME(Simple to avoid pulling caused by root motion blended) BEGIN
		const float theSpeedRatio = FMath::Max(GetMaxSpeed() / 600.f, 1.f);
		const float theCorrectionDistance = NetworkBlendRootMotionClientCorrectionDistance * theSpeedRatio;
		if (LocDiff.Size() < theCorrectionDistance)
		{
			return false;
		}
		//@VIRTUALGAME(Simple to avoid pulling caused by root motion blended) END

		bNetworkLargeClientCorrection |= (LocDiff.SizeSquared() > FMath::Square(NetworkLargeClientCorrectionDistance));
		return true;
	}

	return false;
}
#pragma endregion


#pragma region Rotation
bool UVMM_CharacterMovementComponent::IsUpdateRotationIgnored() const
{
	return (IgnoreUpdatePhysicsCharacterRotation > 0);
}

void UVMM_CharacterMovementComponent::SetIgnoreRotationInput(bool bNewLookInput)
{
	IgnoreUpdatePhysicsCharacterRotation = FMath::Max(IgnoreUpdatePhysicsCharacterRotation + (bNewLookInput ? +1 : -1), 0);
}

void UVMM_CharacterMovementComponent::ResetIgnoreRotationUpdated()
{
	IgnoreUpdatePhysicsCharacterRotation = 0;
}

bool UVMM_CharacterMovementComponent::CanPerformRotation()
{
	check(CharacterOwner);

	// Check the cached state is valid
	if (bCanPerformRotation.IsSet())
	{
		// Cache the value
		const bool theValue = bCanPerformRotation.GetValue();

		// Reset the cached value
		bCanPerformRotation.Reset();

		// Return the result
		return theValue;
	}

	// Check the movement rotation mode is valid
	if (!(bOrientRotationToMovement || bUseControllerDesiredRotation))
	{
		return false;
	}

	// Check the movement rotation locked state
	if (IsUpdateRotationIgnored())
	{
		return false;
	}

	// If haven't any movement input, we don't perform rotation
	if (!HasMovementInput())
	{
		return false;
	}
	
	// Check the movement mode is valid
	if (MovementMode == MOVE_Custom)
	{
		return false;
	}

	// Check the character owner controlled state
	if (!CharacterOwner->IsLocallyControlled())
	{
		return false;
	}

	// Check the velocity size
	if (!IsHorizontalMoving())
	{
		return false;
	}

	// Success
	return true;
}

void UVMM_CharacterMovementComponent::CalculateDesiredRotation(FRotator& OutRotation)
{
	// Define the adjust rotation
	FRotator theAdjustRotation = FRotator::ZeroRotator;

	// Get cached virtual input rotation, full the weight
	// We should always output the rotation that finally needs to be offset according to the animation weight
	float theWeightSize = DirectionWeight.Size();
	const FVector4 theWeightDir = (theWeightSize > SMALL_NUMBER) ? (DirectionWeight / theWeightSize) : FVector4(FVector(1.f), 1.f);
	DirectionWeight = theWeightDir;

	// Check rotation mode
	switch (GetRotationMode())
	{
	case ERotationMode::Relaxed:
	case ERotationMode::UnEasy:
	case ERotationMode::Combat:
		switch (MovementDirection)
		{
		case EAngleDirection::ForwardLeft:
			theAdjustRotation.Yaw = (-45.f * DirectionWeight.X);
			break;
		case EAngleDirection::ForwardRight:
			theAdjustRotation.Yaw = (45.f * DirectionWeight.X);
			break;
		case EAngleDirection::BackwardLeft:
			theAdjustRotation.Yaw = (45.f * DirectionWeight.Y);
			break;
		case EAngleDirection::BackwardRight:
			theAdjustRotation.Yaw = (-45.f * DirectionWeight.Y);
			break;
		}
	break;

	case ERotationMode::Velocity:
		theAdjustRotation = MovementRotation;
		break;
	}

	// Compose input rotation
	OutRotation = (theAdjustRotation.Quaternion() * OutRotation.Quaternion()).Rotator();
}

void UVMM_CharacterMovementComponent::PhysicsRotation(float DeltaSeconds)
{
#if 1
	// If is playing root motion, we should sync the custom rotation
	if (true || HasAnimRootMotion())
	{
		CharacterRotation = UpdatedComponent->GetComponentQuat();
	}
#endif

#if 1
	// Always cache the root motion additive rotation
	if (HasAnimRootMotion())
	{
		const FQuat RootMotionRotationQuat = RootMotionParams.GetRootMotionTransform().GetRotation();
		if (!RootMotionRotationQuat.IsIdentity())
		{
			LastRootMotionRotation = RootMotionRotationQuat;
		}
		return;
	}
#endif

	// Check perform rotation condition
	if (!CanPerformRotation())
	{
		return;
	}

	PhysicsRotationFrame = GFrameCounter;

	// Calculate desired character rotation
	FRotator theDesiredRotation = CharacterOwner->GetControlRotation();
	CalculateDesiredRotation(theDesiredRotation);

	// Set desired rotation
	PhysicsDesiredRotation(DeltaSeconds, theDesiredRotation);
}

void UVMM_CharacterMovementComponent::PhysicsDesiredRotation(const float& InDeltaSeconds, const FRotator& InDesiredRotation
	, const float* InIncrementalAngle, FQuat* OutRotation)
{
	// Get current character rotation
	FRotator theCurrentRotation = CharacterRotation.Rotator();

	// Calculate desired character rotation
	FRotator theDesiredRotation = InDesiredRotation;

	// Adjust rotation axis
	if (ShouldRemainVertical())
	{
		theDesiredRotation.Pitch = 0.f;
		theDesiredRotation.Yaw = FRotator::NormalizeAxis(theDesiredRotation.Yaw);
		theDesiredRotation.Roll = 0.f;
	}
	else
	{
		theDesiredRotation.Normalize();
	}

	// Accumulate a desired new rotation.
	const float AngleTolerance = 1e-3f;

	// Update character rotation
	if (!theCurrentRotation.Equals(theDesiredRotation, AngleTolerance))
	{
		// Set the new rotation.
		theDesiredRotation.DiagnosticCheckNaN(TEXT("UVMM_CharacterMovementComponent::PhysicsRotation(): DesiredRotation"));

#if 1 // Choose additive rotation mode
		// Calculate the remaining angle
		const float theRemainingAngle = UKismetMathLibrary::NormalizedDeltaRotator(theDesiredRotation, theCurrentRotation).Yaw;

		// Get the circle angle additive value
		const float theAnimIncrementalAngle = InIncrementalAngle != nullptr ? *InIncrementalAngle : RotationRate.Yaw;

		// Calculate the frame max additive value, we always use 60FPS in setting
		const float theMaxIncrementalAngle = theAnimIncrementalAngle * (InDeltaSeconds / (1.f / 60.f));

		// Calculate the additive angle in the frame
		float theAdditiveAngle = FMath::Clamp(theRemainingAngle, -theMaxIncrementalAngle, theMaxIncrementalAngle);

		// Calculate the foot offset alpha
		const float theFootOffsetAlpha = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 10.f), FVector2D(0.f, 1.f), Velocity.Size());

		// Blend the additive angle weight
		// The constraint of left and right rotation should be added.
		// If it's a right foot offset, we can allow 90 degrees to the right and 45 degrees to the left
		theAdditiveAngle *= theFootOffsetAlpha;

		// Use procedural additive rotation
		if (RotationScaleRange != FVector2D(1.f))
		{
			const float theScale = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 180.f), FVector2D(RotationScaleRange.X, RotationScaleRange.Y), FMath::Abs(theRemainingAngle));
			theAdditiveAngle *= theScale;
		}

		// If is playing root motion, we should sync the custom rotation
		if (HasAnimRootMotion())
		{
			const FRotator theMotionRotation = RootMotionParams.GetRootMotionTransform().Rotator();
			if (!FMath::IsNearlyEqual(0.f, theMotionRotation.Yaw, 0.01f))
			{
				CharacterRotation = UpdatedComponent->GetComponentQuat();
			}
		}

#if 0
		FString theDebugString = FString::Printf(TEXT("UVMM_CharacterMovementComponent::PhysicsDesiredRotation:: Additive: %f, MaxAngle: %f, FootOffsetAlpha: %f")
			, theAdditiveAngle, theMaxIncrementalAngle, theFootOffsetAlpha);
		UKismetSystemLibrary::PrintString(this, theDebugString, true, true, FLinearColor::Red, 0.f);
#endif

		// Check the final additive angle is valid
		if (FMath::Abs(theAdditiveAngle) <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		// Calculate desired rotation
		FQuat theNewRotation = UKismetMathLibrary::ComposeRotators(theCurrentRotation, FRotator(0.f, theAdditiveAngle, 0.f)).Quaternion();

		// Calculate delta updated rotation in last frame
		const FRotator theDeltaUpdatedRotation = (GetLastUpdateQuat().Inverse() * theNewRotation).Rotator();

		// Choose smooth rotation?
		if (true || FMath::Abs(theDeltaUpdatedRotation.Yaw) > 2.f)
		{
			// Calculate new rotation
			theNewRotation = FMath::QInterpTo(CharacterRotation, theNewRotation, InDeltaSeconds, 250.f);
		}
#else
		FQuat theNewRotation = FMath::QInterpTo(CharacterRotation, FQuat(theDesiredRotation), InDeltaSeconds, RotationRate.Yaw);
#endif

		// Notify character change the rotation
		if (OutRotation == nullptr)
		{
			SetCharacterRotation(true, theNewRotation);
		}
		else
		{
			*OutRotation = theNewRotation;
		}
	}
}

void UVMM_CharacterMovementComponent::AddCharacterRotation(const bool InLocallyControlled, const FQuat& InAdditiveRotation)
{
	SetCharacterRotation(InLocallyControlled, (InAdditiveRotation * CharacterRotation));
}

void UVMM_CharacterMovementComponent::SetCharacterRotation(const bool InLocallyControlled, const FQuat& InDesiredRotation)
{
	// Check the custom character owner is valid
	if (VMM_CharacterOwner == nullptr)
	{
		return;
	}

	// Response character rotation changed
	VMM_CharacterOwner->SetCharacterRotation(InLocallyControlled, InDesiredRotation);
}

bool UVMM_CharacterMovementComponent::OnSetCharacterRotation(const FQuat& InRotation)
{
	// Cache the rotation
	CharacterRotation = InRotation;

	// Rotate the root component
	return MoveUpdatedComponent(FVector::ZeroVector, InRotation, false);
}

ERotationMode UVMM_CharacterMovementComponent::GetRotationMode() const
{
	return VMM_CharacterOwner ? VMM_CharacterOwner->GetRotationMode() : ERotationMode::Velocity;
}

const FQuat UVMM_CharacterMovementComponent::GetCharacterDeltaQuat()
{
	const FQuat theLastUpdatedQuat = GetLastUpdateQuat();
	return (theLastUpdatedQuat.Inverse() * CharacterRotation);
}
#pragma endregion


#pragma region Movement Input
void UVMM_CharacterMovementComponent::SetMovementInput(bool InHasMovement)
{
	// Response condition changed only
	if (bHasMovementInput == InHasMovement)
	{
		return;
	}

	bHasMovementInput = InHasMovement;
}

void UVMM_CharacterMovementComponent::ResetMovementDirection()
{
	LastMovementDirection = MovementDirection;
	LastMovementCardinalDirection = MovementCardinalDirection;
}

void UVMM_CharacterMovementComponent::ResponseMovementDirectionChanged()
{
	// Check the direction delegates is bound
	if (!MovementDirectionDelegate.IsBound())
	{
		return;
	}

	// Response delegate callback
#if 0
	MovementDirectionDelegate.Broadcast(LastMovementDirection, MovementDirection
		, LastMovementCardinalDirection, MovementCardinalDirection, LastMovementRotation, MovementRotation);
#else
	MovementDirectionDelegate.Broadcast(this);
#endif
}

void UVMM_CharacterMovementComponent::ResponseInputDirection(
	const EInputDirection& InLastInputDirection, const EInputDirection& InInputDirection
	, const FRotator& InLastInputRotation, const FRotator& InInputRotation)
{
	// Convert the input direction to angle direction
	const EAngleDirection theMovementDirection = UVMM_Library::ConvertToAngleDirection(InInputDirection, InLastInputRotation);

#if 0 // Always response any input changed now.
	// Response direction changed only
	if (MovementDirection == theMovementDirection)
	{
		return;
	}
#endif

	// Cache the last direction data
	LastMovementRotation = MovementRotation;
	LastMovementDirection = MovementDirection;
	LastMovementCardinalDirection = MovementCardinalDirection;

	// Cache current direction data
	MovementDirection = theMovementDirection;
	MovementCardinalDirection = UVMM_Library::ConvertToCardinalDirection(theMovementDirection);

#if 0
	// If is velocity rotation, we always use forward direction animation
	if (true && MovementCardinalDirection != ECardinalDirection::MAX)
	{
		MovementCardinalDirection = ECardinalDirection::Forward;
	}
#endif

	// Determine
	switch (theMovementDirection)
	{
	case EAngleDirection::ForwardLF:
	case EAngleDirection::ForwardRF:
		MovementRotation.Yaw = 0.f;
		break;

	case EAngleDirection::ForwardLeft:
		MovementRotation.Yaw = -45.f;
		break;
	case EAngleDirection::ForwardRight:
		MovementRotation.Yaw = 45.f;
		break;
	case EAngleDirection::Left:
		MovementRotation.Yaw = -90.f;
		break;
	case EAngleDirection::Right:
		MovementRotation.Yaw = 90.f;
		break;
	case EAngleDirection::BackwardLeft:
		MovementRotation.Yaw = -135.f;
		break;
	case EAngleDirection::BackwardRight:
		MovementRotation.Yaw = 135.f;
		break;
	case EAngleDirection::BackwardLF:
		MovementRotation.Yaw = -180.f;
		break;
	case EAngleDirection::BackwardRF:
		MovementRotation.Yaw = 180.f;
		break;
	}

	// On movement input changed
	SetMovementInput(theMovementDirection != EAngleDirection::MAX);

	// Apply cached movement direction
	ResponseMovementDirectionChanged();
}

void UVMM_CharacterMovementComponent::ResponseMovementAbilityInput(bool InTrigger)
{
	// Response delegate callback
	if (MovementAbilityDelegate.IsBound())
	{
		MovementAbilityDelegate.Broadcast(this, InTrigger);
	}
}

void UVMM_CharacterMovementComponent::K2_ResponseInputDirection(EInputDirection InInputDirection)
{
	ResponseInputDirection(EInputDirection::MAX, InInputDirection, LastMovementRotation, MovementRotation);
}

const FIntPoint UVMM_CharacterMovementComponent::GetGaitAxisRange() const
{
	return VMM_CharacterOwner ? VMM_CharacterOwner->GetGaitAxisRange() : FIntPoint(0);
}

FRotator UVMM_CharacterMovementComponent::GetAdjustMovementRotation() const
{
	// If is velocity rotation mode, we use movement rotation
	if (GetRotationMode() == ERotationMode::Velocity)
	{
		return GetMovementRotation();
	}

	// Choose movement direction
	switch (MovementDirection)
	{
	case EAngleDirection::ForwardLeft:
		return FRotator(0.f, -45.f, 0.f);

	case EAngleDirection::ForwardRight:
		return FRotator(0.f, 45.f, 0.f);

	case EAngleDirection::BackwardLeft:
		return FRotator(0.f, 45.f, 0.f);

	case EAngleDirection::BackwardRight:
		return FRotator(0.f, -45.f, 0.f);
	}

	// INVALID
	return FRotator::ZeroRotator;
}

ECardinalDirection UVMM_CharacterMovementComponent::GetAdjustMovementCardinalDirection() const
{
	return (HasMovementInput() && GetRotationMode() == ERotationMode::Velocity) ? ECardinalDirection::Forward : GetMovementCardinalDirection();
}

ECardinalDirection UVMM_CharacterMovementComponent::GetAdjustLastMovementCardinalDirection() const
{
	return (GetRotationMode() == ERotationMode::Velocity) ? ECardinalDirection::Forward : GetLastMovementCardinalDirection();
}

#pragma endregion