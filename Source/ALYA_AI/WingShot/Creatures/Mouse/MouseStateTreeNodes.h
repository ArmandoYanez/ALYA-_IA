// Nodos de StateTree exclusivos del raton: caminar su ruta y huir.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MouseStateTreeNodes.generated.h"

class AAIController;

// =====================================================================
// Raton: camina al otro extremo de su ruta (A <-> B) por el NavMesh.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeTask_MouseWalkRouteInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	// Punto de la ruta al que va.
	FVector Destination = FVector::ZeroVector;
};

USTRUCT(meta = (DisplayName = "Mouse Walk Route", Category = "WingShot|Mouse"))
struct ALYA_AI_API FStateTreeTask_MouseWalkRoute : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_MouseWalkRouteInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// =====================================================================
// Raton: huye corriendo en direccion contraria a la amenaza percibida.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeTask_MouseFleeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	// Que tan lejos corre.
	float FleeDistance = 900.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	// Velocidad al huir.
	float FleeSpeed = 480.f;

	// Velocidad normal (para regresarla al terminar).
	float PreviousSpeed = 0.f;
};

USTRUCT(meta = (DisplayName = "Mouse Flee", Category = "WingShot|Mouse"))
struct ALYA_AI_API FStateTreeTask_MouseFlee : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_MouseFleeInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
