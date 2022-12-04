// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputTriggers.h"
#include "InputActionValue.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "VMM_InputTrigger.generated.h"

class AActor;
class UEnhancedPlayerInput;

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_InputTrigger : public UInputTrigger
{
	GENERATED_UCLASS_BODY()

public:

	/** Flag bind ability input id. - It is usually marked in the input action. If the engine is not modified, this is an alternative. */
	UPROPERTY(VisibleDefaultsOnly, Category = Action)
	int32 InputID;

	/** Flag use gameplay event trigger **/
	UPROPERTY(EditDefaultsOnly, Category = "Ability Settings")
	uint8 bUseGameplayEvent : 1;

#if 0
	/** Flag use add loose gameplay tag **/
	UPROPERTY(EditDefaultsOnly, Category = "Ability Settings")
	uint8 bUseLooseGameplayEvent : 1;
#endif

	/** Trigger gameplay tag */
	UPROPERTY(EditDefaultsOnly, Category = "Ability Settings")
	TArray<FGameplayTag> Tags;

	/** Trigger gameplay ability. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability Settings", meta = (EditCondition = "!bUseGameplayEvent"))
	TArray<TSubclassOf<UGameplayAbility>> Abilities;

public:

	/** Set bind input id from global manager */
	void SetInputID(const int32& InInputID);

	/** Return match other input trigger */
	bool MatchTrigger(const UVMM_InputTrigger* InOtherTrigger) const;

	/** Return has bind input id condition */
	FORCEINLINE bool HasInputID() const { return InputID >= 0; }

	/** Return bind input id */
	FORCEINLINE const int32& GetInputID() const { return InputID; }

	/** Return use gameplay event abilities */
	FORCEINLINE bool UseGameplayEvent() const { return bUseGameplayEvent; }

#if 0
	/** Return use add loose gameplay event */
	FORCEINLINE bool UseLooseGameplayEvent() const { return bUseLooseGameplayEvent; }
#endif

	/** Return the tag is valid */
	FORCEINLINE bool HasTags() const { return Tags.Num() > 0 && Tags[0].IsValid(); }

	/** Return the ability is valid */
	FORCEINLINE bool HasAbilities() const { return Abilities.Num() > 0 && Abilities[0] != NULL; }
};

/** UVMM_InputTriggerPressed
	Trigger fires once only when input exceeds the actuation threshold.
	Holding the input will not cause further triggers.
	*/
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "Ability Pressed"))
class UVMM_InputTriggerPressed final : public UVMM_InputTrigger
{
	GENERATED_BODY()

protected:

	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime) override;
	virtual FString GetDebugState() const { return IsActuated(LastValue) ? FString(TEXT("Pressed:Held")) : FString(); }
};


/** UVMM_InputTriggerReleased
	Trigger returns Ongoing whilst input exceeds the actuation threshold.
	Trigger fires once only when input drops back below actuation threshold.
	*/
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "Ability Released"))
class UVMM_InputTriggerReleased final : public UVMM_InputTrigger
{
	GENERATED_BODY()

protected:

	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime) override;
	virtual FString GetDebugState() const { return IsActuated(LastValue) ? FString(TEXT("Released:Held")) : FString(); }
};