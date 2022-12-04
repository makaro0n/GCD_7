// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "VMM_Types.h"
#include "VMM_InputMappingContext.h"
#include "VMM_InputComponent.generated.h"

class APlayerController;
class UVMM_InputTrigger;
class UVMM_InputAction;
class UVMM_InputMappingContext;
class UAbilitySystemComponent;

#define EnableDelayResponsePlayerInput				1

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FInputDirectionSignature
, const EInputDirection&, InLastInputDirection, const EInputDirection&, InInputDirection
, const FRotator&, InLastInputRotation, const FRotator&, InInputRotation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovementAbilityInputSignature, bool, InTrigger);

/**
 * UVMM_InputComponent
 *
 *	Component used to manage input mappings and bindings using an input config data asset.
 */
UCLASS(Config = Input)
class UVMM_InputComponent : public UEnhancedInputComponent
{
	GENERATED_UCLASS_BODY()

protected:

	UPROPERTY(Transient)
	APlayerController* PlayerController;

	/** Cache ability input action map of input ID */
	UPROPERTY(Transient)
	TArray<const UVMM_InputAction*> AbilityInputActions;

#pragma region Initialize

public:

	/** Return input trigger data */
	virtual const UVMM_InputTrigger* GetInputTrigger(const FInputActionInstance& InInputActionInstance);

	/** Initialize player input mapping context */
	virtual void InitializePlayerInput(APlayerController* InPlayerController, UEnhancedInputLocalPlayerSubsystem* InSubsSystem, const UVMM_InputMappingContext* InMappingContext);

#pragma endregion


#pragma region Ability

	/** Try activate the trigger ability */
	virtual void TryActivateAbilities(AActor* InActor, const FInputActionInstance& InInputActionInstance, const UVMM_InputTrigger* InInputTrigger);

	/** Try activate the trigger ability */
	virtual bool TryActivateAbilitiesInputID(UAbilitySystemComponent* InAbilityComponent, const FInputActionInstance& InInputActionInstance, const UVMM_InputTrigger* InInputTrigger);

	/** Response bind ability input event */
	virtual void ResponseAbilityEvent(const FInputActionInstance& InInputActionInstance);

#pragma endregion


#pragma region Look

protected:

	/** View input rate config */
	FVector2D ViewInputRate;

protected:

	/** Response look input */
	void Input_LookMode(const FInputActionValue& InInputActionValue);
	void Input_LookMouse(const FInputActionValue& InInputActionValue);
	void Input_LookYaw(const FInputActionValue& InInputActionValue);
	void Input_LookPitch(const FInputActionValue& InInputActionValue);
	void Input_LookStick(const FInputActionValue& InInputActionValue);
	void Input_LookDistance(const FInputActionValue& InInputActionValue);

public:

	/** Player look left right rotation input. */
	void ResponseLookYaw(float InValue);

	/** Player look up down rotation input. */
	void ResponseLookPitch(float InValue);

#pragma endregion


#pragma region Movement Input

private:

	/** Cached delay response input timer */
	FTimerHandle DelayResponseInputTimer;

	/** X is forward axis, Y is right axis. */
	FVector2D MoveAxis;

	/** Whether it is in pressed forward. */
	uint8 bPressedForward : 1;

	/** Whether it is in pressed backward. */
	uint8 bPressedBackward : 1;

	/** Whether it is in pressed left. */
	uint8 bPressedLeft : 1;

	/** Whether it is in pressed right. */
	uint8 bPressedRight : 1;

	/** The current rotation pressed. */
	FRotator InputRotation;

	/** The last rotation pressed. */
	FRotator LastInputRotation;

	/** The last valid rotation pressed. */
	FRotator CachedValidInputRotation;

	/** The current direction pressed. */
	EInputDirection InputDirection;

	/** The last direction pressed. */
	EInputDirection LastInputDirection;

protected:

	/** Set player move axis input. */
	void SetMoveAxisValue(const bool& InForwardAixs, const float& InAxisValue);

	/** Calculate the input direction based on the player's axis input */
	bool SetInputDirection();

	/** Calculate the input direction based on the player's axis input */
	EInputDirection GetPlayerInputDirection(FRotator* OutRotation = nullptr);

	/** Calculate the input direction based on the player's axis input */
	float GetPlayerInputAngle();
	
	/** Response movement input */
	void Input_Move(const FInputActionInstance& InInputActionInstance);
	void Input_StopMove(const FInputActionInstance& InInputActionInstance);
	void Input_MoveFoward(const FInputActionInstance& InInputActionInstance);
	void Input_MoveBackward(const FInputActionInstance& InInputActionInstance);
	void Input_MoveLeft(const FInputActionInstance& InInputActionInstance);
	void Input_MoveRight(const FInputActionInstance& InInputActionInstance);
	void Input_MoveAbility(const FInputActionInstance& InInputActionInstance);

protected:

	/** Player move key input. */
	void ResponseMovementInput(const FInputActionInstance& InInputActionInstance, EInputDirection InInputDirection);

	/** Response cached input direction key */
	void OnResponseCachedInputDirection();

public:

	/** Called input direction changed */
	FInputDirectionSignature InputDirectionDelegate;

	/** Called movement input ability changed */
	FMovementAbilityInputSignature MovementAbilityInputDelegate;

	/** Returns whether the move axis value is valid. */
	FORCEINLINE bool IsValidMoveAxis()const { return IsValidForwardAxis() || IsValidRightAxis(); }

	/** Returns whether the axis value is valid. */
	FORCEINLINE bool IsValidForwardAxis()const { return MoveAxis.X != 0.f; }

	/** Returns whether the axis value is valid. */
	FORCEINLINE bool IsValidRightAxis() const { return MoveAxis.Y != 0.f; }

	/** Returns whether there is player key input. */
	FORCEINLINE bool HasPressedInput() const { return bPressedForward || bPressedBackward || bPressedLeft || bPressedRight; }

	/** Returns cached valid input rotation. */
	FORCEINLINE FRotator GetValidInputRotation() const { return CachedValidInputRotation; }

#pragma endregion
};