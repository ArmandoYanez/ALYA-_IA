#include "StateTreeTask_Patrol.h"

#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "VisualLogger/VisualLogger.h"
#include "IA/StateTreeAIUtils.h"

EStateTreeRunStatus FStateTreeTask_Patrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.AIController)
	{
		InstanceData.AIController = ALYA::StateTree::ResolveAIController(Context.GetOwner());
	}

	APawn* Pawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		UE_VLOG_UELOG(Context.GetOwner(), LogStateTree, Error, TEXT("Patrol: no hay AIController o Pawn."));
		return EStateTreeRunStatus::Failed;
	}

	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(Pawn, InstanceData.PatrolTag, TaggedActors);
	TaggedActors.Sort([](const AActor& A, const AActor& B) { return A.GetName() < B.GetName(); });

	InstanceData.PatrolPoints.Reset();
	for (const AActor* Actor : TaggedActors)
	{
		InstanceData.PatrolPoints.Add(Actor->GetActorLocation());
	}

	InstanceData.PointIndex = INDEX_NONE;
	if (InstanceData.PatrolPoints.Num() > 0)
	{
		const FVector PawnLocation = Pawn->GetActorLocation();
		int32 Closest = 0;
		for (int32 i = 1; i < InstanceData.PatrolPoints.Num(); ++i)
		{
			if (FVector::DistSquared(PawnLocation, InstanceData.PatrolPoints[i]) < FVector::DistSquared(PawnLocation, InstanceData.PatrolPoints[Closest]))
			{
				Closest = i;
			}
		}
		InstanceData.PointIndex = Closest - 1;
	}
	else
	{
		UE_VLOG_UELOG(Context.GetOwner(), LogStateTree, Warning,
			TEXT("Patrol: no hay actores con el tag '%s', se usara deambular aleatorio."), *InstanceData.PatrolTag.ToString());
	}

	InstanceData.WaitRemaining = 0.0f;
	MoveToNextPoint(Context, InstanceData);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_Patrol::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.AIController || !InstanceData.AIController->GetPawn())
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.bIsMoving)
	{
		if (InstanceData.AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
		{
			return EStateTreeRunStatus::Running;
		}

		InstanceData.bIsMoving = false;
		InstanceData.WaitRemaining = InstanceData.WaitTimeAtPoint;
		UE_VLOG_LOCATION(Context.GetOwner(), LogStateTree, Log,
			InstanceData.CurrentPatrolLocation, 30.0f, FColor::Cyan, TEXT("Patrol: llego al punto, esperando"));
	}

	InstanceData.WaitRemaining -= DeltaTime;
	if (InstanceData.WaitRemaining <= 0.0f)
	{
		MoveToNextPoint(Context, InstanceData);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_Patrol::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.bIsMoving && InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
	InstanceData.bIsMoving = false;
}

void FStateTreeTask_Patrol::MoveToNextPoint(FStateTreeExecutionContext& Context, FInstanceDataType& InstanceData) const
{
	const APawn* Pawn = InstanceData.AIController->GetPawn();

	bool bHasDestination = false;
	if (InstanceData.PatrolPoints.Num() > 0)
	{
		InstanceData.PointIndex = (InstanceData.PointIndex + 1) % InstanceData.PatrolPoints.Num();
		InstanceData.CurrentPatrolLocation = InstanceData.PatrolPoints[InstanceData.PointIndex];
		bHasDestination = true;
	}
	else if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld()))
	{
		FNavLocation RandomLocation;
		if (NavSys->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), InstanceData.WanderRadius, RandomLocation))
		{
			InstanceData.CurrentPatrolLocation = RandomLocation.Location;
			bHasDestination = true;
		}
	}

	if (!bHasDestination)
	{
		UE_VLOG_UELOG(Context.GetOwner(), LogStateTree, Warning, TEXT("Patrol: no se encontro destino (revisa el NavMesh)."));
		InstanceData.WaitRemaining = InstanceData.WaitTimeAtPoint;
		return;
	}

	const EPathFollowingRequestResult::Type Result = InstanceData.AIController->MoveToLocation(
		InstanceData.CurrentPatrolLocation, InstanceData.AcceptanceRadius);

	InstanceData.bIsMoving = (Result == EPathFollowingRequestResult::RequestSuccessful);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		UE_VLOG_UELOG(Context.GetOwner(), LogStateTree, Warning, TEXT("Patrol: MoveTo fallo hacia %s."), *InstanceData.CurrentPatrolLocation.ToString());
	}

	if (!InstanceData.bIsMoving)
	{
		InstanceData.WaitRemaining = InstanceData.WaitTimeAtPoint;
	}
	else
	{
		UE_VLOG_LOCATION(Context.GetOwner(), LogStateTree, Log,
			InstanceData.CurrentPatrolLocation, 30.0f, FColor::Blue, TEXT("Patrol: yendo al punto"));
	}
}
