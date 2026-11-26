// Nodos de StateTree exclusivos del buho

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "OwlStateTreeNodes.generated.h"

// Duerme un tiempo aleatorio (buho). Mientras duerme solo lo despierta un ruido fuerte
USTRUCT()
struct ALYA_AI_API FStateTreeTask_BirdSleepInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MinTime = 8.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MaxTime = 15.f;

	// tiempo que falta para despertar
	float Remaining = 0.f;
};

USTRUCT(meta = (DisplayName = "Bird Sleep", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeTask_BirdSleep : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_BirdSleepInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
