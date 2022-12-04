// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "Engine/DataAsset.h"

#include "VMM_AbilitySet.generated.h"


class UVMM_AbilitySystemComponent;
class UVMM_GameplayAbility;
class UGameplayEffect;


/**
 * FVirtualAbilitySet_GameplayAbility
 *
 *	Data used by the ability set to grant gameplay abilities.
 */
USTRUCT(BlueprintType)
struct FVirtualAbilitySet_GameplayAbility
{
	GENERATED_BODY()

public:

	// Level of ability to grant.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	bool bInitializeActive = false;

	// Gameplay ability to grant.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TSubclassOf<UGameplayAbility> Ability = nullptr;

	// Level of ability to grant.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	int32 AbilityLevel = 1;

	// Tag used to process input for the ability.
	UPROPERTY(EditDefaultsOnly, Category = "Data", Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};


/**
 * FVirtualAbilitySet_GameplayEffect
 *
 *	Data used by the ability set to grant gameplay effects.
 */
USTRUCT(BlueprintType)
struct FVirtualAbilitySet_GameplayEffect
{
	GENERATED_BODY()

public:

	// Gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	// Level of gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float EffectLevel = 1.0f;
};

/**
 * FVirtualAbilitySet_AttributeSet
 *
 *	Data used by the ability set to grant attribute sets.
 */
USTRUCT(BlueprintType)
struct FVirtualAbilitySet_AttributeSet
{
	GENERATED_BODY()

public:
	// Gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TSubclassOf<UAttributeSet> AttributeSet;

};

/**
 * FVirtualAbilitySet_GrantedHandles
 *
 *	Data used to store handles to what has been granted by the ability set.
 */
USTRUCT(BlueprintType)
struct FVirtualAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);
	void AddAttributeSet(UAttributeSet* Set);

	void TakeFromAbilitySystem(UVMM_AbilitySystemComponent* VMM_ASC);

protected:

	// Handles to the granted abilities.
	UPROPERTY()
		TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	// Handles to the granted gameplay effects.
	UPROPERTY()
		TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	// Pointers to the granted attribute sets
	UPROPERTY()
		TArray<UAttributeSet*> GrantedAttributeSets;
};


/**
 * UVMM_AbilitySet
 *
 *	Non-mutable data asset used to grant gameplay abilities and gameplay effects.
 */
UCLASS(BlueprintType, Const)
class VIRTUALMOTIONMATCHING_API UVMM_AbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UVMM_AbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Grants the ability set to the specified ability system component.
	// The returned handles can be used later to take away anything that was granted.
	void GiveToAbilitySystem(UVMM_AbilitySystemComponent* InASC, FVirtualAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

protected:

	// Gameplay abilities to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta = (TitleProperty = Ability))
		TArray<FVirtualAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	// Gameplay effects to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta = (TitleProperty = GameplayEffect))
		TArray<FVirtualAbilitySet_GameplayEffect> GrantedGameplayEffects;

	// Attribute sets to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Attribute Sets", meta = (TitleProperty = AttributeSet))
		TArray<FVirtualAbilitySet_AttributeSet> GrantedAttributes;
};
