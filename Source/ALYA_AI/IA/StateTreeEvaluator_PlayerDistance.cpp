#include "StateTreeEvaluator_PlayerDistance.h"

#include "StateTreeExecutionContext.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

void FStateTreeEvaluator_PlayerDistance::TreeStart(FStateTreeExecutionContext& Context) const
{
	UpdateDistance(Context);
}

void FStateTreeEvaluator_PlayerDistance::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UpdateDistance(Context);
}

void FStateTreeEvaluator_PlayerDistance::UpdateDistance(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const APawn* Pawn = InstanceData.Pawn;
	const APawn* Player = Pawn ? UGameplayStatics::GetPlayerPawn(Pawn, 0) : nullptr;

	if (!Pawn || !Player)
	{
		InstanceData.DistanceToPlayer = TNumericLimits<float>::Max();
		InstanceData.bIsPlayerNear = false;
		return;
	}

	InstanceData.DistanceToPlayer = FVector::Dist(Pawn->GetActorLocation(), Player->GetActorLocation());
	InstanceData.bIsPlayerNear = InstanceData.DistanceToPlayer < InstanceData.DistanceThreshold;
}
