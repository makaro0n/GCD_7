// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "VMM_Types.h"
#include "VMM_InputMappingContext.generated.h"

class UDataTable;
class UVMM_InputAction;
class UGameplayTagsManager;

/**
 * FInputDefinition
 *
 *	Struct used to map a input action to a gameplay input tag.
 */
USTRUCT(BlueprintType)
struct FInputDefinition
{
	GENERATED_BODY()

	/** Input action data asset */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	const UVMM_InputAction* Action;
#if 0
	/** Key that affect the action. */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
		FKey Key;

	/** Input gameplay tag */
	UPROPERTY(VisibleDefaultsOnly, Meta = (Categories = "Data"))
		FGameplayTag Tag;

	/** Gameplay ability to grant. */
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Data"))
		TSubclassOf<UGameplayAbility> Ability;

#if WITH_EDITORONLY_DATA
	/** Input gameplay tag, we will initialize in editor, @See InitializeMappingContext */
	UPROPERTY(EditDefaultsOnly, Category = "Editor")
		FName TagName;

	/** Input gameplay tag describe, we will initialize in editor, @See InitializeMappingContext */
	UPROPERTY(EditDefaultsOnly, Category = "Editor")
		FString DevComment;
#endif // WITH_EDITORONLY_DATA
#endif
public:

	/** Return the input action is valid */
	FORCEINLINE bool IsValid() const { return Action != NULL; }

	FInputDefinition()
		: Action(NULL)
	{}
};

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_InputMappingContext : public UInputMappingContext
{
	GENERATED_UCLASS_BODY()

#pragma region Config

public:

	/** The angle configuration range parameter of input direction - MoveAxis. */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	TMap<EInputDirection, FVector2D> InputDirectionRangeMap;

	/** View input rate config */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FVector2D ViewInputRate;

	/** View yaw limit angle range */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FVector2D ViewYawLimitAngle;

	/** View pitch limit angle range */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FVector2D ViewPitchLimitAngle;

#pragma endregion

public:

	/** Basic look input definitions, look yaw, pitch etc. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data", Meta = (TitleProperty = "InputAction"))
	TArray<FInputDefinition> LookInputDefinitions;

	/** Basic movement input definitions, Common, Forward, Backward, Left, Right */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, EditFixedSize, Category = "Data", Meta = (TitleProperty = "InputAction"))
	TArray<FInputDefinition> MovementInputDefinitions;

	/** Basic movement ability input definitions, Jump... */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data", Meta = (TitleProperty = "InputAction"))
	TArray<FInputDefinition> MovementAbilityInputDefinitions;

	// List of input actions used by the owner.  These input actions are mapped to a gameplay tag and must be manually bound.
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data", Meta = (TitleProperty = "InputAction"))
	TArray<FInputDefinition> NativeInputDefinitions;

	// List of input actions used by the owner.  These input actions are mapped to a gameplay tag and are automatically bound to abilities with matching input tags.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data", Meta = (TitleProperty = "InputAction"))
	TArray<FInputDefinition> AbilityInputDefinitions;

#if WITH_EDITORONLY_DATA
	/** We transfer Input definitions to the gameplay row table, which are then automatically added to GameplaySettings */
	//UPROPERTY(EditDefaultsOnly, Category = "Editor")
	UDataTable* GameplayTagDataTable;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	/**
	 * Called when a property on this object has been modified externally
	 *
	 * @param PropertyThatChanged the property that was modified
	*/
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& InPropertyChangedEvent) override;
#endif //WITH_EDITOR

#if 0
	/** We will find the corresponding Input Tag according to the Input tag name for automatic configuration */
	//UFUNCTION(CallInEditor, Category = "Editor")
	void InitializeInputTag();

	/** We copy the Input tag, input dev comment to the specified DataTable, and then link the DataTable to GameplaySettings */
	//UFUNCTION(CallInEditor, Category = "Editor")
	void InitializeDataTable();
#endif

	/** We copy the Input action, input key to mapping context */
	UFUNCTION(CallInEditor, Category = "Editor")
	void InitializeMappingContext();
#endif // WITH_EDITOR
};
