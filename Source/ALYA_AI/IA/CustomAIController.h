// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Components/StateTreeComponent.h"
#include "CustomAIController.generated.h"

/*
Alojar componente de los state trees
*/
UCLASS()
class ALYA_AI_API ACustomAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACustomAIController();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;
};