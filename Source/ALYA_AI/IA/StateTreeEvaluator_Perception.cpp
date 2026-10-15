#include "StateTreeEvaluator_Perception.h"

#include "StateTreeExecutionContext.h"
#include "IA/CustomAIController.h"
#include "IA/StateTreeAIUtils.h"

void FStateTreeEvaluator_Perception::TreeStart(FStateTreeExecutionContext& Context) const
{
	ReadPerception(Context);
}

void FStateTreeEvaluator_Perception::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	ReadPerception(Context);
}

void FStateTreeEvaluator_Perception::ReadPerception(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.AIController)
	{
		InstanceData.AIController = ALYA::StateTree::ResolveAIController(Context.GetOwner());
	}

	const ACustomAIController* Controller = Cast<ACustomAIController>(InstanceData.AIController);
	if (!Controller)
	{
		InstanceData.bCanSeePlayer = false;
		return;
	}

	InstanceData.bCanSeePlayer = Controller->CanSeePlayer();
	InstanceData.PlayerActor = Controller->GetPerceivedPlayer();
	InstanceData.LastKnownPlayerLocation = Controller->GetLastKnownPlayerLocation();
}
