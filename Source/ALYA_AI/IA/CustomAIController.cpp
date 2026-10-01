// Fill out your copyright notice in the Description page of Project Settings.

#include "IA/CustomAIController.h"

ACustomAIController::ACustomAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>("StateTreeComponent");
}