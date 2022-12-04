// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VMM_Types.h"
#include "UObject/WeakInterfacePtr.h"
#include "GameFramework/PlayerController.h"
#include "VMM_PlayerController.generated.h"

class UVMM_InputComponent;

/**
 *
 */
UCLASS()
class VIRTUALMOTIONMATCHING_API AVMM_PlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()

protected:

	/** Cached local player reference */
	UPROPERTY()
	UVMM_InputComponent* EnhancedInputComponent;

protected:
	virtual void SetPawn(APawn* InPawn) override;
public:
	virtual void SetupInputComponent() override;

#pragma region Input

public:

	/** Common input mappings */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UVMM_InputMappingContext* InputMappingContext;

#pragma endregion
};
