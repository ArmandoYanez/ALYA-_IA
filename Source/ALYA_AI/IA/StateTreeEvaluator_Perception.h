#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeEvaluator_Perception.generated.h"

class AAIController;

USTRUCT()
struct ALYA_AI_API FStateTreeEvaluator_PerceptionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bCanSeePlayer = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> PlayerActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	FVector LastKnownPlayerLocation = FVector::ZeroVector;
};

USTRUCT(meta = (DisplayName = "AI Perception (Sight)", Category = "AI|Perception"))
struct ALYA_AI_API FStateTreeEvaluator_Perception : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEvaluator_PerceptionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	void ReadPerception(FStateTreeExecutionContext& Context) const;
};
