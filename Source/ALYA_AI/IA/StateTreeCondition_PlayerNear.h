#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeCondition_PlayerNear.generated.h"

class APawn;

USTRUCT()
struct ALYA_AI_API FStateTreeCondition_PlayerNearInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Distance = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;
};

USTRUCT(meta = (DisplayName = "Is Player Near", Category = "AI|Perception"))
struct ALYA_AI_API FStateTreeCondition_PlayerNear : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_PlayerNearInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
