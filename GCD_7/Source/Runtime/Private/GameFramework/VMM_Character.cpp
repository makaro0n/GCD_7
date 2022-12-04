// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "GameFramework/VMM_Character.h"
#include "GameFramework/VMM_CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Abilities/VMM_AbilitySystemComponent.h"
#include "Librarys/VMM_Library.h"
#include "Input/VMM_InputComponent.h"
#include "GameFramework/PlayerController.h"

#include "Net/UnrealNetwork.h"
#include <GameFramework/CharacterMovementComponent.h>
AVMM_Character::AVMM_Character(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVMM_CharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Avoid ticking characters if possible.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	NetCullDistanceSquared = 900000000.0f;

	UCapsuleComponent* theCapsuleComponent = GetCapsuleComponent();
	check(theCapsuleComponent);
	theCapsuleComponent->InitCapsuleSize(32.f, 90.0f);

	USkeletalMeshComponent* theMeshComponent = GetMesh();
	check(theMeshComponent);
	theMeshComponent->SetRelativeRotation(FRotator(0.0f, -92.5f, 0.0f));  // Rotate mesh to be X forward since it is exported as Y forward.

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	BaseEyeHeight = 80.0f;
	CrouchedEyeHeight = 50.0f;

	// Create ability system component
	VirtualAbilitySystemComponent = CreateDefaultSubobject<UVMM_AbilitySystemComponent>(TEXT("VirtualAbilitySystemComponent"));
	VirtualAbilitySystemComponent->SetIsReplicated(true);

	// Static cast animation skill character movement
	CharacterMovementComponent = Cast<UVMM_CharacterMovementComponent>(GetCharacterMovement());

	// Gait
	DefaultGaitAxis = 1;
	PressedGaitAxis = 1;
}

UVMM_AbilitySystemComponent* AVMM_Character::GetVMMAbilitySystemComponent() const
{
	return VirtualAbilitySystemComponent;
}

UAbilitySystemComponent* AVMM_Character::GetAbilitySystemComponent() const
{
	return VirtualAbilitySystemComponent;
}

void AVMM_Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AVMM_Character, GaitAxis, COND_SimulatedOnly);

	// If we are not using root motion locomotion, we don't need to perform the copy here
	DOREPLIFETIME_CONDITION(AVMM_Character, RotationYaw, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AVMM_Character, RotationMode, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AVMM_Character, ControlRotationPack, COND_SimulatedOnly);
}

// Called when the game starts or when spawned
void AVMM_Character::BeginPlay()
{
	Super::BeginPlay();

	// Set tick sort.
	if (GetMesh())
	{
		GetMesh()->AddTickPrerequisiteActor(this);
	}

	// Initialize our abilities
	if (VirtualAbilitySystemComponent && GetLocalRole() == ROLE_SimulatedProxy)
	{
		VirtualAbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	// Initialize gait axis
	OnGaitAxisChanged(DefaultGaitAxis);
}

void AVMM_Character::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

	// Initialize our abilities
	if (VirtualAbilitySystemComponent)
	{
		VirtualAbilitySystemComponent->InitAbilityActorInfo(NewController, this);
		VirtualAbilitySystemComponent->InitializeAbilityData(NewController, this);
	}
}

void AVMM_Character::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// We should use the montage created by the simulator itself, because onrep_ RootMotion cannot inherit, so it cannot continue...
	if (RepRootMotion.AnimMontage != nullptr)
	{
		if (VirtualAbilitySystemComponent && VirtualAbilitySystemComponent->GetSkillMontage() == RepRootMotion.AnimMontage)
		{
			RepRootMotion.AnimMontage = nullptr;
		}
	}
}

#pragma region Input
void AVMM_Character::DestroyPlayerInputComponent()
{
	// Unbind input delegates
	if (APlayerController* thePlayerController = Cast<APlayerController>(GetController()))
	{
		if (UVMM_InputComponent* theInputComponent = Cast<UVMM_InputComponent>(thePlayerController->InputComponent))
		{
			theInputComponent->InputDirectionDelegate.RemoveDynamic(CharacterMovementComponent, &UVMM_CharacterMovementComponent::ResponseInputDirection);
			theInputComponent->MovementAbilityInputDelegate.RemoveDynamic(CharacterMovementComponent, &UVMM_CharacterMovementComponent::ResponseMovementAbilityInput);
		}
	}

	Super::DestroyPlayerInputComponent();
}

void AVMM_Character::SetupPlayerInputComponent(UInputComponent* InPlayerInputComponent)
{
	Super::SetupPlayerInputComponent(InPlayerInputComponent);

	// Bind input delegates
	if (APlayerController* thePlayerController = Cast<APlayerController>(GetController()))
	{
		if (UVMM_InputComponent* theInputComponent = Cast<UVMM_InputComponent>(thePlayerController->InputComponent))
		{
			theInputComponent->InputDirectionDelegate.AddUniqueDynamic(CharacterMovementComponent, &UVMM_CharacterMovementComponent::ResponseInputDirection);
			theInputComponent->MovementAbilityInputDelegate.AddUniqueDynamic(CharacterMovementComponent, &UVMM_CharacterMovementComponent::ResponseMovementAbilityInput);
		}
	}
}
#pragma endregion


#pragma region Rotation
void AVMM_Character::OnRep_RotationMode()
{
	OnRotationModeChanged(RotationMode);
}

void AVMM_Character::OnRep_RotationYaw()
{
	OnCharacterRotationYawChanged(RotationYaw);
}

void AVMM_Character::OnRep_ControlRotation()
{
	// View components
	const uint16 theViewPitch = (ControlRotationPack & 65535);
	const uint16 theViewYaw = (ControlRotationPack >> 16);
	
	// Convert to rotator
	SimulateControlRotation.Pitch = FRotator::DecompressAxisFromShort(theViewPitch);
	SimulateControlRotation.Yaw = FRotator::DecompressAxisFromShort(theViewYaw);
}

bool AVMM_Character::OnRotationModeChanged(const ERotationMode& InRotationMode)
{
	// Cache valid looking rotation mode
	LastLookingRotationMode = IsLookingRotationMode() ? RotationMode : LastLookingRotationMode;

	// Cache current rotation mode
	LastRotationMode = RotationMode;

	// Cache new rotation mode
	RotationMode = InRotationMode;

	// If is bound delegate, we should notify it.
	if (RotationModeDelegate.IsBound())
	{
		RotationModeDelegate.Broadcast(RotationMode, LastRotationMode, LastLookingRotationMode);
	}

	// Success
	return true;
}

void AVMM_Character::OnCharacterRotationYawChanged(uint16 InRotationYaw)
{
	// Get the source character rotation
	FRotator theCharacterRotation = CharacterMovementComponent->GetCharacterRotation();

	// Replace the rotation yaw
	theCharacterRotation.Yaw = FRotator::NormalizeAxis(FRotator::DecompressAxisFromShort(InRotationYaw));

	// We calculate the rotation and placement of moving components that need to be changed
	CharacterMovementComponent->OnSetCharacterRotation(theCharacterRotation.Quaternion());

	/** Character Compression Rotation Yaw SimulatedOnly, The default server will copy actor rotation, so we don't have to, See @SmoothCorrection */
}

void AVMM_Character::FaceRotation(FRotator InControlRotation, float InDeltaTime)
{
	Super::FaceRotation(InControlRotation, InDeltaTime);

	// If is server face, we will replicated rotation to simulate
	if (GetNetMode() != NM_Standalone && HasAuthority())
	{
		// Compress rotation down to 2 - 4 bytes
		ControlRotationPack = UCharacterMovementComponent::PackYawAndPitchTo32(InControlRotation.Yaw, InControlRotation.Pitch);
		MARK_PROPERTY_DIRTY_FROM_NAME(AVMM_Character, ControlRotationPack, this);
	}

	// Call face rotation delegate
	if (FaceRotationDelegate.IsBound())
	{
		FaceRotationDelegate.Broadcast(InDeltaTime, InControlRotation);
	}
}

void AVMM_Character::AddCharacterRotation(bool InLocallyControlled, const FRotator InAdditiveRotation)
{
	SetCharacterRotation(InLocallyControlled, (FQuat(InAdditiveRotation) * CharacterMovementComponent->GetCharacterQuat()));
}

void AVMM_Character::SetCharacterRotation(bool InLocallyControlled, const FQuat& InDesiredRotation)
{
	// We already have additional objects, so we don't need to set our own rotation
	if (GetAttachParentActor() != nullptr)
	{
		return;
	}

	// We calculate the rotation and placement of moving components that need to be changed
	const bool bHasRotate = CharacterMovementComponent->OnSetCharacterRotation(InDesiredRotation);

	// If the player calls the function, we should send it to the server
	if (bHasRotate && InLocallyControlled && UVMM_Library::IsAutonomousProxy(this))
	{
#if 1 // Choose compress size
		const uint16& CompressRotationYaw = FRotator::CompressAxisToShort(InDesiredRotation.Rotator().Yaw);
#else	
		const uint8& CompressRotationYaw = FRotator::CompressAxisToByte(InDesiredRotation.Rotator().Yaw);
#endif

		// Send to server
		ServerSetCharacterRotationYaw(CompressRotationYaw);
	}
}

bool AVMM_Character::SetRotationMode(const ERotationMode& InRotationMode)
{
	// Check the new rotation mode is valid
	if (RotationMode == InRotationMode)
	{
		return false;
	}

	// Response the rotation mode changed
	const bool bHasChanged = OnRotationModeChanged(InRotationMode);

	// Return the result
	return bHasChanged;
}

void AVMM_Character::ServerSetCharacterRotationYaw_Implementation(uint16 InRotationYaw)
{
	OnCharacterRotationYawChanged(InRotationYaw);

	// If we are not using root motion locomotion, we don't need to perform the copy here
	RotationYaw = InRotationYaw;
	MARK_PROPERTY_DIRTY_FROM_NAME(AVMM_Character, RotationYaw, this);
}

bool AVMM_Character::ServerSetCharacterRotationYaw_Validate(uint16 InRotationYaw)
{
	return true;
}

FRotator AVMM_Character::GetLookingRotation()
{
	// If is simulate proxy, we should convert rotation pack
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		return SimulateControlRotation;
	}

	// Return the result
	return GetControlRotation();
}

const FRotator AVMM_Character::GetCharacterRotation() const
{
	return CharacterMovementComponent->GetCharacterRotation();
}
#pragma endregion


#pragma region Stance
bool AVMM_Character::CanCrouch() const
{
	// Check the parent condition
	if (Super::CanCrouch() == false)
	{
		return false;
	}

	// Check the movement state
	if (CharacterMovementComponent && !CharacterMovementComponent->IsMovingOnGround())
	{
		return false;
	}

	// Return the result
	return true;
}

void AVMM_Character::OnStartCrouch(float InHalfHeightAdjust, float InScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(InHalfHeightAdjust, InScaledHalfHeightAdjust);

	// Notify delegates
	if (CharacterStanceDelegate.IsBound())
	{
		CharacterStanceDelegate.Broadcast(this);
	}
}

void AVMM_Character::OnEndCrouch(float InHalfHeightAdjust, float InScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(InHalfHeightAdjust, InScaledHalfHeightAdjust);

	// Notify delegates
	if (CharacterStanceDelegate.IsBound())
	{
		CharacterStanceDelegate.Broadcast(this);
	}
}
#pragma endregion


#pragma region Gait
void AVMM_Character::OnRep_GaitAxis()
{
	OnGaitAxisChanged(GaitAxis);
}

void AVMM_Character::SetGaitAxis(const uint8& InGaitAxis)
{
	// Always cache pressed gait axis
	PressedGaitAxis = InGaitAxis;

	// Check the new gait axis is valid
	if (InGaitAxis == GaitAxis)
	{
		return;
	}

	// Response gait axis changed
	OnGaitAxisChanged(InGaitAxis);
}

void AVMM_Character::OnGaitAxisChanged(const uint8& InGaitAxis)
{
	LastGaitAxis = GaitAxis;
	GaitAxis = InGaitAxis;

	// Notify delegates
	if (CharacterGaitDelegate.IsBound())
	{
		CharacterGaitDelegate.Broadcast(this);
	}
}

const FIntPoint AVMM_Character::GetGaitAxisRange() const
{
	// If attribute set is valid, we try get the cached data
// 	if (HasAbilitySysComp())
// 	{
// 		return FIntPoint(GaitAxis, AbilitySystemComponent->GetGaitParamsIndex(GetAnimStateIndex(), GaitAxis));
// 	}

	// Return default range
	return FIntPoint(GaitAxis, INDEX_NONE);
}
#pragma endregion