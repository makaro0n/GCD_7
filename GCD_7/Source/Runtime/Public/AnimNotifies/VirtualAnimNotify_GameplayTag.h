// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "VirtualAnimNotify_GameplayTag.generated.h"

/**
 * 
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API UVirtualAnimNotify_GameplayTag : public UAnimNotify
{
	GENERATED_UCLASS_BODY()
	
protected:

	/** Gameplay tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimNotify)
	FGameplayTag Tag;

	/** Gameplay event data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimNotify)
	FGameplayEventData Event;

public:

#if ENGINE_MAJOR_VERSION > 4
	/** Response notify event */
	virtual void Notify(USkeletalMeshComponent* InMeshComp, UAnimSequenceBase* InAnimation, const FAnimNotifyEventReference& InEventReference);
#else
	/** Response notify event */
	virtual void Notify(USkeletalMeshComponent* InMeshComp, UAnimSequenceBase* InAnimation) override;
#endif // ENGINE_MAJOR_VERSION > 4
};
