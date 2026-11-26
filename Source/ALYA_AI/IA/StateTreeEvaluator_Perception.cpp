#include "StateTreeEvaluator_Perception.h"

#include "StateTreeExecutionContext.h"
#include "IA/CustomAIController.h"

void FStateTreeEvaluator_Perception::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// convierte el AIController generico a nuestro controller para leer su percepcion
	const ACustomAIController* Controller = Cast<ACustomAIController>(InstanceData.AIController);

	// Copia los valores que el controller guardo en OnTargetPerceptionUpdated (no calcula nada aqui)
	InstanceData.bHasTarget = Controller && Controller->HasTarget();
	InstanceData.TargetActor = Controller ? Controller->GetTargetActor() : nullptr;
}
