// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "StateTreeTask_FindCover.generated.h"

class UEnvQuery;
class AAIController;

USTRUCT()
struct ALYA_AI_API FStateTreeTask_FindCoverInstanceData
{
	GENERATED_BODY()
	
	//Entrada: Query a ejecutar
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> CoverQuery = nullptr;

	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	//Salida: Ubicación encontrada (Output permite enlazar a variables del StateTree)
	UPROPERTY(EditAnywhere, Category = "Output", meta = (Output))
	FVector CoverLocation = FVector::ZeroVector;

	//Estado interno
	UPROPERTY()
	EStateTreeRunStatus ExecutionStatus = EStateTreeRunStatus::Running;
};

USTRUCT(meta = (DisplayName = "Find Cover (EQS)", Category = "AI|Cover"))
struct ALYA_AI_API FStateTreeTask_FindCover : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_FindCoverInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};