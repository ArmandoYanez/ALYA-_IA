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

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UEnvQuery> CoverQuery = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bMoveToCover = true;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bMoveToCover", ClampMin = "10.0", UIMin = "10.0"))
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Output")
	FVector CoverLocation = FVector::ZeroVector;

	int32 QueryRequestID = INDEX_NONE;

	bool bIsMoving = false;

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
