#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeEvaluator_PlayerDistance.generated.h"

class APawn;

USTRUCT()
struct ALYA_AI_API FStateTreeEvaluator_PlayerDistanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DistanceThreshold = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Output")
	float DistanceToPlayer = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsPlayerNear = false;
};

USTRUCT(meta = (DisplayName = "Player Distance", Category = "AI|Perception"))
struct ALYA_AI_API FStateTreeEvaluator_PlayerDistance : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEvaluator_PlayerDistanceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	void UpdateDistance(FStateTreeExecutionContext& Context) const;
};
