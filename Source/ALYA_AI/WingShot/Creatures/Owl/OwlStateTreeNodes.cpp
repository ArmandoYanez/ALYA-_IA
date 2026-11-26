// Tarea Sleep del buho: duerme un rato y luego despierta.

#include "WingShot/Creatures/Owl/OwlStateTreeNodes.h"

#include "StateTreeExecutionContext.h"
#include "WingShot/Creatures/Shared/BirdBase.h"

// ---------------------------------------------------------------------
// Sleep
// ---------------------------------------------------------------------
EStateTreeRunStatus FStateTreeTask_BirdSleep::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Remaining = FMath::FRandRange(Data.MinTime, FMath::Max(Data.MinTime, Data.MaxTime));
	if (ABirdBase* Bird = Cast<ABirdBase>(Data.Pawn))
	{
		// Marca al ave como dormida (el Blueprint lo lee con IsSleeping).
		Bird->SetSleeping(true);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_BirdSleep::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	// Cuenta regresiva; en 0 despierta.
	Data.Remaining -= DeltaTime;
	return Data.Remaining <= 0.f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FStateTreeTask_BirdSleep::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (ABirdBase* Bird = Cast<ABirdBase>(Context.GetInstanceData(*this).Pawn))
	{
		// Despierta.
		Bird->SetSleeping(false);
	}
}
