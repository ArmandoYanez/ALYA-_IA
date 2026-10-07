#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeTask_Patrol.generated.h"

class AAIController;

USTRUCT()
struct ALYA_AI_API FStateTreeTask_PatrolInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName PatrolTag = TEXT("Patrol");

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WaitTimeAtPoint = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "10.0", UIMin = "10.0"))
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float WanderRadius = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Output")
	FVector CurrentPatrolLocation = FVector::ZeroVector;

	TArray<FVector> PatrolPoints;

	int32 PointIndex = INDEX_NONE;

	float WaitRemaining = 0.0f;

	bool bIsMoving = false;
};

USTRUCT(meta = (DisplayName = "Patrol", Category = "AI|Movement"))
struct ALYA_AI_API FStateTreeTask_Patrol : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_PatrolInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	void MoveToNextPoint(FStateTreeExecutionContext& Context, FInstanceDataType& InstanceData) const;
};
