// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Librarys/VMM_AbilityLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/VMM_CharacterMovementComponent.h"


void UVMM_AbilityLibrary::ResponseCachedMovementInput(UVMM_CharacterMovementComponent* InMovement)
{
	if (InMovement)
	{
		InMovement->ResponseMovementDirectionChanged();
	}
}

bool UVMM_AbilityLibrary::AddLooseGameplayTags(AActor* Actor, const FGameplayTagContainer& GameplayTags, bool bShouldReplicate)
{
	if (UAbilitySystemComponent* AbilitySysComp = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
	{
		AbilitySysComp->AddLooseGameplayTags(GameplayTags);

#if ENGINE_MAJOR_VERSION > 4
		if (bShouldReplicate)
		{
			AbilitySysComp->AddReplicatedLooseGameplayTags(GameplayTags);
		}
#endif // ENGINE_MAJOR_VERSION > 4
		return true;
	}

	return false;
}

bool UVMM_AbilityLibrary::RemoveLooseGameplayTags(AActor* Actor, const FGameplayTagContainer& GameplayTags, bool bShouldReplicate)
{
	if (UAbilitySystemComponent* AbilitySysComp = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
	{
		AbilitySysComp->RemoveLooseGameplayTags(GameplayTags);

#if ENGINE_MAJOR_VERSION > 4
		if (bShouldReplicate)
		{
			AbilitySysComp->RemoveReplicatedLooseGameplayTags(GameplayTags);
		}
#endif // ENGINE_MAJOR_VERSION > 4

		return true;
	}

	return false;
}