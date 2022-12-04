// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "VMM_Character.generated.h"

class UAbilitySystemComponent;
class UVMM_AbilitySystemComponent;
class UVMM_CharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterGaitSignature, AVMM_Character*, InCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterStanceSignature, ACharacter*, InCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFaceRotationSignature, const float&, InDeltaSeconds, const FRotator&, InControlRotation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRotationModeSignature, const ERotationMode&, InRotationMode, const ERotationMode&, InLastRotationMode, const ERotationMode&, InLastLookingRotationMode);


UCLASS()
class VIRTUALMOTIONMATCHING_API AVMM_Character : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVMM_Character(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Custom Movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	UVMM_CharacterMovementComponent* CharacterMovementComponent;

	/** Pointer to the ability system component that is cached for convenience. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Ability)
	UVMM_AbilitySystemComponent* VirtualAbilitySystemComponent;

	/** Return ability system component from virtual motion matching */
	UFUNCTION(BlueprintCallable, Category = "VMM|Character")
	UVMM_AbilitySystemComponent* GetVMMAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void PossessedBy(AController* NewController) override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

#pragma region Input

	/** Destroys the player input component and removes any references to it. */
	virtual void DestroyPlayerInputComponent() override;

	/** Allows a Pawn to set up custom input bindings. Called upon possession by a PlayerController, using the InputComponent created by CreatePlayerInputComponent(). */
	virtual void SetupPlayerInputComponent(UInputComponent* InPlayerInputComponent) override;

#pragma endregion


#pragma region Rotation

protected:

	/** Array of tags that can be used for grouping and categorizing. Can also be accessed from scripting. Combat, Relaxed, UnEasy, Velocity, etc. */
	UPROPERTY(ReplicatedUsing = OnRep_RotationMode)
	ERotationMode RotationMode;

	/** Cached the rotation mode */
	ERotationMode LastRotationMode;

	/** Cached last looking rotation mode */
	ERotationMode LastLookingRotationMode;

	/** Replicated version of character rotation. Read-only on simulated proxies! */
	UPROPERTY(ReplicatedUsing = OnRep_RotationYaw)
	uint16 RotationYaw;

	/** Replicated version of character control rotation. Read-only on simulated proxies! */
	UPROPERTY(ReplicatedUsing = OnRep_ControlRotation)
	uint32 ControlRotationPack;

	/** Cache controller rotation converted by network compressed packets, @See::OnRep_ControlRotation */
	FRotator SimulateControlRotation;

	/** Called when Character Rotation Mode is replicated */
	UFUNCTION()
	virtual void OnRep_RotationMode();

	/** Handles replicated character rotation properties on simulated proxies.
	  * If is playing root motion, we need it. @See::OnRep_RootMotion, @See::OnRep_ReplicatedMovement */
	UFUNCTION()
	virtual void OnRep_RotationYaw();

	/** Handles replicated character control rotation properties on simulated proxies. */
	UFUNCTION()
	virtual void OnRep_ControlRotation();

	/**
	 *The modification of application character rotation mode is usually applied by non-local end.
	 *
	 * @param InRotationMode	Character rotation mode index.
	 */
	virtual bool OnRotationModeChanged(const ERotationMode& InRotationMode);

	/**
	 *The modification of application character rotation is usually applied by non-local end.
	 *
	 * @param InCompressionRotationYaw	Compressed 16-byte rotated yaw.
	 */
	void OnCharacterRotationYawChanged(uint16 InRotationYaw);

public:

	/** Updates Pawn's rotation to the given rotation, assumed to be the Controller's ControlRotation. Respects the bUseControllerRotation* settings. */
	virtual void FaceRotation(FRotator InControlRotation, float InDeltaTime = 0.f) override;

	/**
	 * Set the desired character rotation.
	 *
	 * @param InLocallyControlled		If it is the master, we should send the request to the server, where we can merge into the ServerMove of the mobile component.
	 * @param InDesiredRotation		Desired rotation (quaternion will be faster)
	 */
	UFUNCTION(BlueprintCallable, Category = Rotation)
	void AddCharacterRotation(bool InLocallyControlled, const FRotator InAdditiveRotation);

	/**
	 * Set the desired character rotation.
	 *
	 * @param InLocallyControlled		If it is the master, we should send the request to the server, where we can merge into the ServerMove of the mobile component.
	 * @param InDesiredRotation		Desired rotation (quaternion will be faster)
	 */
	UFUNCTION(BlueprintCallable, Category = Rotation)
	void SetCharacterRotation(bool InLocallyControlled, const FQuat& InDesiredRotation);

	/** Set new rotation mode */
	virtual bool SetRotationMode(const ERotationMode& InRotationMode);

private:

	/** We should merge into ServerMovePack to prevent too fast server messages from causing network congestion */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetCharacterRotationYaw(uint16 InRotationYaw);

public:

	/** Called rotation mode type changed */
	FRotationModeSignature RotationModeDelegate;

	/** Called face rotation changed */
	FFaceRotationSignature FaceRotationDelegate;

	/** Returns the current character controller rotation */
	FRotator GetLookingRotation();

	/** Returns the current character rotation, which is smooth relative to actor rotation */
	const FRotator GetCharacterRotation() const;

	/** Return the rotation mode is Velocity */
	FORCEINLINE bool IsVelocityRotationMode() { return RotationMode == ERotationMode::Velocity; }

	/** Return the rotation mode is Looking */
	FORCEINLINE bool IsLookingRotationMode() { return RotationMode <= ERotationMode::Combat; }

	/** Return the rotation mode is Combat */
	FORCEINLINE bool IsCombatRotationMode() { return RotationMode == ERotationMode::Combat; }

	/** Returns the current rotation mode */
	FORCEINLINE const ERotationMode& GetRotationMode() const { return RotationMode; }

	/** Returns last rotation mode */
	FORCEINLINE const ERotationMode& GetLastRotationMode() const { return LastRotationMode; }

	/** Returns last looking rotation mode */
	FORCEINLINE const ERotationMode& GetLastLookingRotationMode() const { return LastLookingRotationMode; }

public:

	/** Returns the current character rotation mode */
	UFUNCTION(BlueprintPure, Category = "Rotation", meta = (DisplayName = "Get Rotation Mode"))
	ERotationMode K2_GetRotationMode() { return RotationMode; }

	/** Returns the current character control rotation */
	UFUNCTION(BlueprintPure, Category = "Rotation", meta = (DisplayName = "Get Looking Rotation"))
	FRotator K2_GetLookingRotation() { return GetLookingRotation(); }

	/** Returns the current character rotation, which is smooth relative to actor rotation */
	UFUNCTION(BlueprintPure, Category = "Rotation", meta = (DisplayName = "Get Character Rotation"))
	FRotator K2_GetCharacterRotation() { return GetCharacterRotation(); }

#pragma endregion


#pragma region Stance

public:

	/** Character stance delegates */
	FCharacterStanceSignature CharacterStanceDelegate;

	/** @return true if this character is currently able to crouch (and is not currently crouched) */
	virtual bool CanCrouch() const override;

protected:


	/**
	 * Called when Character crouches. Called on non-owned Characters through bIsCrouched replication.
	 * @param	HalfHeightAdjust		difference between default collision half-height, and actual crouched capsule half-height.
	 * @param	ScaledHalfHeightAdjust	difference after component scale is taken in to account.
	 */
	virtual void OnStartCrouch(float InHalfHeightAdjust, float InScaledHalfHeightAdjust) override;

	/**
	 * Called when Character stops crouching. Called on non-owned Characters through bIsCrouched replication.
	 * @param	HalfHeightAdjust		difference between default collision half-height, and actual crouched capsule half-height.
	 * @param	ScaledHalfHeightAdjust	difference after component scale is taken in to account.
	 */
	virtual void OnEndCrouch(float InHalfHeightAdjust, float InScaledHalfHeightAdjust) override;

#pragma endregion


#pragma region Gait

protected:

	/** Represents the gait of locomotion speed */
	UPROPERTY(EditDefaultsOnly, Category = "Gait")
	uint8 MaxGaitAxis;

	/** Represents the gait of locomotion speed */
	UPROPERTY(EditDefaultsOnly, Category = "Gait")
	uint8 DefaultGaitAxis;

	/** Represents the gait of locomotion speed */
	UPROPERTY(ReplicatedUsing = OnRep_GaitAxis)
	uint8 GaitAxis;

	/** Cached last gait axis */
	uint8 LastGaitAxis;

	/** Cached last pressed gait axis */
	uint8 PressedGaitAxis;

	/** Called when Locomotion Gait is replicated */
	UFUNCTION()
	void OnRep_GaitAxis();

public:

	/** Set character locomotion gait axis */
	void SetGaitAxis(const uint8& InGaitAxis);

	/** On Change locomotion gait axis from last pressed gait axis */
	void OnPressedGaitAxis() { OnGaitAxisChanged(PressedGaitAxis); }

	/** On Change locomotion gait axis */
	virtual void OnGaitAxisChanged(const uint8& InGaitAxis);

public:

	/** Character gait delegates */
	FCharacterGaitSignature CharacterGaitDelegate;

	/** Returns the character locomotion gait axis range */
	const FIntPoint GetGaitAxisRange() const;

	/** Return current gait axis */
	FORCEINLINE const uint8& GetGaitAxis() const { return GaitAxis; }

	/** Return last gait axis */
	FORCEINLINE const uint8& GetLastGaitAxis() const { return LastGaitAxis; }

	/** Return support max gait axis */
	FORCEINLINE const uint8& GetMaxGaitAxis() const { return MaxGaitAxis; }

	/*-----------------------------------------------------------------------------
		Blueprint Callable.
	-----------------------------------------------------------------------------*/

	/** Returns the character locomotion gait axis */
	UFUNCTION(BlueprintPure, Category = "Gait", meta = (DisplayName = "Get Gait Axis"))
	FORCEINLINE uint8 K2_GetGaitAxis() const { return GaitAxis; }

	/** Set character locomotion gait axis */
	UFUNCTION(BlueprintCallable, Category = "Gait", meta = (DisplayName = "Set Gait Axis"))
	void K2_SetGaitAxis(uint8 InGaitAxis) { SetGaitAxis(InGaitAxis); }

#pragma endregion
};
