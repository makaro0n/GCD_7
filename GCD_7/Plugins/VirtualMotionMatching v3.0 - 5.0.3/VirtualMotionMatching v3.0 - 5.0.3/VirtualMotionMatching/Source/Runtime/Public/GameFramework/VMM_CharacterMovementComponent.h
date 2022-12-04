// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "VMM_Types.h"
#include "VMM_CharacterMovementComponent.generated.h"

class AVMM_Character;

#if 0
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FMovementDirectionSignature
, const EAngleDirection&, InLastMoveDirection, const EAngleDirection&, InMoveDirection
, const ECardinalDirection&, InLastMoveCardinalDirection, const ECardinalDirection&, InMoveCardinalDirection
, const FRotator&, InLastMoveRotation, const FRotator&, InMoveRotation);
#else
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovementDirectionSignature, UVMM_CharacterMovementComponent*, InMovementComp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMovementAbilitySignature, UVMM_CharacterMovementComponent*, InMovementComp, bool, InTrigger);
#endif

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_CharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

protected:

	/** Character movement component belongs to */
	UPROPERTY(Transient, DuplicateTransient)
	AVMM_Character* VMM_CharacterOwner;

#pragma region Owner

public:

	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PostLoad() override;
	//End UActorComponent Interface

	/** Initialize updated component */
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;

	/** Perform movement on an autonomous client */
	virtual void PerformMovement(float InDeltaTime) override;

	/**
	 * React to new transform from network update. Sets bNetworkSmoothingComplete to false to ensure future smoothing updates.
	 * IMPORTANT: It is expected that this function triggers any movement/transform updates to match the network update if desired.
	 */
	virtual void SmoothCorrection(const FVector& OldLocation, const FQuat& OldRotation, const FVector& NewLocation, const FQuat& NewRotation) override;

	/**
	 * Check position error within ServerCheckClientError(). Set bNetworkLargeClientCorrection to true if the correction should be prioritized (delayed less in SendClientAdjustment).
	 */
	virtual bool ServerExceedsAllowablePositionError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode) override;

#pragma endregion


#pragma region Network

protected:

	/**
	* Since the engine currently does not support the blending of root bone montages, we need to use this parameter to avoid the server frequently correcting the client's position.
	* It should scale dynamically based on the animation max speed
	* @see ServerExceedsAllowablePositionError
	*/
	UPROPERTY(Category = "Character Movement (Networking)", EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NetworkBlendRootMotionClientCorrectionDistance;

#pragma endregion


#pragma region Rotation

protected:

	/** The rotation value of the current character is usually controlled locally, which is more timely and smooth than ActorRotation */
	FQuat CharacterRotation;

	/** Ignores update physicas rotation. Stacked state storage, Use accessor function IgnoreMoveInput() */
	UPROPERTY(Transient)
	uint8 IgnoreUpdatePhysicsCharacterRotation;

	/** Mark a frame to update only one rotation */
	uint64 PhysicsRotationFrame;

	/** Flag perform rotation state */
	TOptional<bool> bCanPerformRotation;

	/** Last simulate root motion rotation @See::PhysicsRotation() */
	TOptional<FQuat> LastRootMotionRotation;

	/** Rotation additive scale. X - Y: 0 - 180, Z - W: 1 - X */
	UPROPERTY(Category = "Character Movement (Rotation Settings)", EditAnywhere, BlueprintReadWrite)
	FVector2D RotationScaleRange;

public:

	/** Returns true if update physics rotation input is ignored. */
	UFUNCTION(BlueprintCallable, Category = "TEST")
	virtual bool IsUpdateRotationIgnored() const;

	/** This indicates that OnCharacterRotationChanged will update actor rotation, then animation rotation not executed */
	virtual void SetIgnoreRotationInput(bool bNewLookInput);

	/** Stops ignoring look input by resetting the update physics rotation state. */
	virtual void ResetIgnoreRotationUpdated();

	/** Decide whether to rotate the character */
	virtual bool CanPerformRotation();

	/** Return desired character rotation */
	virtual void CalculateDesiredRotation(FRotator& OutRotation);

	/** Perform rotation over deltaTime */
	/** Perform movement on an autonomous client */
	virtual void PhysicsRotation(float DeltaTime) override;

	/** Perform rotation over deltaTime */
	/** Perform movement on an autonomous client */
	virtual void PhysicsDesiredRotation(const float& InDeltaSeconds, const FRotator& InDesiredRotation, const float* InIncrementalAngle = nullptr, FQuat* OutRotation = nullptr);

	/**
	 * Updates Character Rotation
	 * This is used internally during movement updates.
	 * @param	InLocallyControlled				is locally controller
	 * @param	InRotation						additive character rotation
	 */
	virtual void AddCharacterRotation(const bool InLocallyControlled, const FQuat& InAdditiveRotation);

	/**
	 * Updates Character Rotation
	 * This is used internally during movement updates.
	 * @param	InLocallyControlled				is locally controller
	 * @param	InRotation						desired character rotation
	 */
	virtual void SetCharacterRotation(const bool InLocallyControlled, const FQuat& InDesiredRotation);

	/**
	 * Updates Character Rotation Changed
	 * This is used internally during movement updates.
	 * @param	InRotation						desired character rotation
	 */
	virtual bool OnSetCharacterRotation(const FQuat& InRotation);

public:

	/** Return the character owner rotation mode */
	ERotationMode GetRotationMode() const;

	/** Return the character owner delta updated rotation */
	const FQuat GetCharacterDeltaQuat();

	/** Return the character owner rotation */
	const FQuat& GetCharacterQuat() const { return CharacterRotation; }

	/** Return the character owner rotation */
	const FRotator GetCharacterRotation() const { return CharacterRotation.Rotator(); }

#pragma endregion


#pragma region Movement Input

protected:

	/** Returns whether there is movement input, from the Controller */
	uint8 bHasMovementInput : 1;

	/** Cached current moving montage cardinal direction weight */
	FVector4 DirectionWeight;

	/** The current local rotation of movement, based on the character direct front */
	FRotator MovementRotation;

	/** The last local rotation of movement, based on the character direct front */
	FRotator LastMovementRotation;

	/** Character current movement direction, based on the character direct front */
	EAngleDirection MovementDirection;

	/** Character last movement direction, based on the character direct front */
	EAngleDirection LastMovementDirection;

	/** Character current movement cardinal direction, based on the character direct front */
	ECardinalDirection MovementCardinalDirection;

	/** Character last movement cardinal direction, based on the character direct front */
	ECardinalDirection LastMovementCardinalDirection;

public:

	/** Called movement direction changed */
	FMovementDirectionSignature MovementDirectionDelegate;

	/** Called movement ability changed */
	FMovementAbilitySignature MovementAbilityDelegate;
	
	/** Determine if there is a movement control input */
	virtual void SetMovementInput(bool InHasMovement);

	/** Reset last movement direction to current direction */
	virtual void ResetMovementDirection();

	/** Response cached movement direction changed */
	virtual void ResponseMovementDirectionChanged();

	/** Response movement input direction changed */
	UFUNCTION()
	virtual void ResponseInputDirection(const EInputDirection& InLastInputDirection, const EInputDirection& InInputDirection
			, const FRotator& InLastInputRotation, const FRotator& InInputRotation);

	/** Response movement input ability changed, Jump, Climb, Vault, etc. */
	UFUNCTION()
	virtual void ResponseMovementAbilityInput(bool InTrigger);

	/** Response movement input direction changed */
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DisplayName = "Response Input Direction"))
	void K2_ResponseInputDirection(EInputDirection InInputDirection);

public:

	/** Returns the character locomotion gait axis range */
	const FIntPoint GetGaitAxisRange() const;

	/** Returns the adjust input rotation of rotation mode */
	UFUNCTION(BlueprintPure, Category = Movement)
	FRotator GetAdjustMovementRotation() const;

	/** Returns whether there is movement input */
	FORCEINLINE bool HasPlayerInput() const { return bHasMovementInput; }

	/** Returns whether there is movement input */
	FORCEINLINE bool HasMovementInput() const { return bHasMovementInput && !IsMoveInputIgnored(); }

	/** Returns whether there is movement input */
	FORCEINLINE bool IsMoving() { return Velocity.Size() > 0.f; }

	/** Returns whether there is Z velocity */
	FORCEINLINE bool IsVerticalMoving() { return Velocity.Z != 0.f; }

	/** Returns whether there is XY velocity */
	FORCEINLINE bool IsHorizontalMoving() { return Velocity.Size2D() > 0.99f; }

	/** Returns the current maximum input speed */
	FORCEINLINE float GetMaxInputSpeed() const { return FMath::Max(GetMaxSpeed() * AnalogInputModifier, GetMinAnalogSpeed()); }

	/** Returns the current movement rotation */
	FORCEINLINE const FRotator& GetMovementRotation() const { return MovementRotation; }

	/** Returns the last movement rotation */
	FORCEINLINE const FRotator& GetLastMovementRotation() const { return LastMovementRotation; }

	/** Returns the current movement direction */
	FORCEINLINE const EAngleDirection& GetMovementDirection() const { return MovementDirection; }

	/** Returns the last movement direction */
	FORCEINLINE const EAngleDirection& GetLastMovementDirection() const { return LastMovementDirection; }

	/** Returns the current movement cardinal direction */
	ECardinalDirection GetAdjustMovementCardinalDirection() const;

	/** Returns the last movement cardinal direction */
	ECardinalDirection GetAdjustLastMovementCardinalDirection() const;

	/** Returns the current movement cardinal direction */
	FORCEINLINE const ECardinalDirection& GetMovementCardinalDirection() const { return MovementCardinalDirection; }

	/** Returns the last movement cardinal direction */
	FORCEINLINE const ECardinalDirection& GetLastMovementCardinalDirection() const { return LastMovementCardinalDirection; }

#pragma endregion
};
