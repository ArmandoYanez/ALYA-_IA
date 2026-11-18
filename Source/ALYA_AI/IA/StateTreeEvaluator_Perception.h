#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeEvaluator_Perception.generated.h"

class AAIController;

USTRUCT()
struct ALYA_AI_API FStateTreeEvaluator_PerceptionInstanceData
{
	GENERATED_BODY()

	// AIController del NPC; se enlaza automaticamente al Context Data del schema.
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	// Salida para el StateTree: true mientras el NPC ve al objetivo.
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasTarget = false;

	// Salida para el StateTree: el objetivo percibido (el jugador).
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;
};

// Evaluator que expone al StateTree lo que detecta el AI Perception del controller.
USTRUCT(meta = (DisplayName = "AI Perception (Sight)", Category = "AI|Perception"))
struct ALYA_AI_API FStateTreeEvaluator_Perception : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEvaluator_PerceptionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
