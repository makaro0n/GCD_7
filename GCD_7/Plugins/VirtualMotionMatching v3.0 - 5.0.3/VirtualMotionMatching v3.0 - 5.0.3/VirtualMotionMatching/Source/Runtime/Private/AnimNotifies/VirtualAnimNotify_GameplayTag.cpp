// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/VirtualAnimNotify_GameplayTag.h"
#include "AbilitySystemBlueprintLibrary.h"

#if WITH_EDITOR
#include "Persona/Public/AnimationEditorPreviewActor.h"
#endif //WITH_EDITOR

UVirtualAnimNotify_GameplayTag::UVirtualAnimNotify_GameplayTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

#if ENGINE_MAJOR_VERSION > 4
void UVirtualAnimNotify_GameplayTag::Notify(USkeletalMeshComponent* InMeshComp, UAnimSequenceBase* InAnimation, const FAnimNotifyEventReference& InEventReference)
{
	Super::Notify(InMeshComp, InAnimation, InEventReference);

#if WITH_EDITOR
	if (InMeshComp->GetOwner() && InMeshComp->GetOwner()->IsA(AAnimationEditorPreviewActor::StaticClass()))
	{
		return;
	}
#endif //WITH_EDITOR

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InMeshComp->GetOwner(), Tag, Event);
}
#else
void UVirtualAnimNotify_GameplayTag::Notify(USkeletalMeshComponent* InMeshComp, UAnimSequenceBase* InAnimation)
{
	Super::Notify(InMeshComp, InAnimation);

#if WITH_EDITOR
	if (InMeshComp->GetOwner() && InMeshComp->GetOwner()->IsA(AAnimationEditorPreviewActor::StaticClass()))
	{
		return;
	}
#endif //WITH_EDITOR

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InMeshComp->GetOwner(), Tag, Event);
}
#endif // ENGINE_MAJOR_VERSION > 4