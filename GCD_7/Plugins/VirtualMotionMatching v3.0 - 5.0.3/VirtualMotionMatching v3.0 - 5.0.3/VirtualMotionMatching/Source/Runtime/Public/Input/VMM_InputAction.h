// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "VMM_InputAction.generated.h"


/**
 * FKeyDefinition
 *
 *	Struct used to map a input action to a gameplay input tag.
 */
USTRUCT(BlueprintType)
struct FKeyDefinition
{
	GENERATED_BODY()

	/** Key that affect the action. */
	UPROPERTY(EditDefaultsOnly, Category = Data)
	FKey Key;

	/**
	* Modifiers are applied to the final action value.
	* These are applied sequentially in array order.
	* They are applied on top of any FEnhancedActionKeyMapping modifiers that drove the initial input
	*/
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = Data)
	TArray<UInputModifier*> Modifiers;

public:

	/** Return the key value is valid */
	FORCEINLINE bool IsValid() const { return Key != EKeys::Invalid; }

	FKeyDefinition()
		: Key(EKeys::Invalid)
	{}
};


/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_InputAction : public UInputAction
{
	GENERATED_UCLASS_BODY()

public:

	/** Flag bind ability input id. */
	UPROPERTY(VisibleAnywhere, Category = Action)
	int32 InputID;

	/** Flag bind ability input id. */
	UPROPERTY(EditDefaultsOnly, Category = Action)
	uint8 bBindInputID : 1;

	/** Keys that affect the action. */
	UPROPERTY(EditDefaultsOnly, Category = Action)
	TArray<FKeyDefinition> KeysDefinition;

#if WITH_EDITOR
protected:
	/**
	 * This alternate version of PostEditChange is called when properties inside structs are modified.  The property that was actually modified
	 * is located at the tail of the list.  The head of the list of the FStructProperty member variable that contains the property that was modified.
	 */
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent) override;
#endif // WITH_EDITOR

public:

	/** Set bind input id from global manager */
	virtual void SetInputID(const int32& InInputID);

	/** Return need bind input id condition */
	FORCEINLINE bool BindInputID() const { return bBindInputID; }

	/** Return need bind input id condition */
	FORCEINLINE const int32& GetInputID() const { return InputID; }
};
