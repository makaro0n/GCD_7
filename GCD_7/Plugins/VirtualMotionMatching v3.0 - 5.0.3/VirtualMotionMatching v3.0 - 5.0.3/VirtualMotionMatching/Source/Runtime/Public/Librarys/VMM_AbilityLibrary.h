// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VMM_AbilityLibrary.generated.h"

class AActor;
class UVMM_CharacterMovementComponent;


/**
 *
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVMM_AbilityLibrary : public UObject
{
	GENERATED_BODY()

public:

	/** If has movement input, we will response movement direction @See UVMM_CharacterMovement::ResponseMovementDirection. */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static void ResponseCachedMovementInput(UVMM_CharacterMovementComponent* InMovement);

	/** Manually adds a set of tags to a given actor, and optionally replicates them. */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static bool AddLooseGameplayTags(AActor* Actor, const FGameplayTagContainer& GameplayTags, bool bShouldReplicate = false);

	/** Manually removes a set of tags from a given actor, with optional replication. */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static bool RemoveLooseGameplayTags(AActor* Actor, const FGameplayTagContainer& GameplayTags, bool bShouldReplicate = false);
};