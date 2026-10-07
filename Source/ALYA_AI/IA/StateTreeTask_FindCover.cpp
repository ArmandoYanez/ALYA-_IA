#include "StateTreeTask_FindCover.h"

#include "StateTreeExecutionContext.h"
#include "StateTreeAsyncExecutionContext.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "VisualLogger/VisualLogger.h"
#include "IA/StateTreeAIUtils.h"

using ALYA::StateTree::ResolveAIController;

EStateTreeRunStatus FStateTreeTask_FindCover::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	InstanceData.ExecutionStatus = EStateTreeRunStatus::Running;
	InstanceData.QueryRequestID = INDEX_NONE;
	InstanceData.bIsMoving = false;

	if (!InstanceData.AIController)
	{
		InstanceData.AIController = ResolveAIController(Context.GetOwner());
	}

	if (!InstanceData.CoverQuery || !InstanceData.AIController)
	{
		UE_VLOG_UELOG(Context.GetOwner(), LogStateTree, Error,
			TEXT("FindCover: falta CoverQuery (%s) o AIController (%s)."),
			*GetNameSafe(InstanceData.CoverQuery), *GetNameSafe(InstanceData.AIController));
		return EStateTreeRunStatus::Failed;
	}

	APawn* ControlledPawn = InstanceData.AIController->GetPawn();
	if (!ControlledPawn)
	{
		UE_VLOG_UELOG(Context.GetOwner(), LogStateTree, Error, TEXT("FindCover: el AIController no posee ningun Pawn."));
		return EStateTreeRunStatus::Failed;
	}

	FEnvQueryRequest QueryRequest(InstanceData.CoverQuery, ControlledPawn);

	InstanceData.QueryRequestID = QueryRequest.Execute(
		InstanceData.RunMode,
		FQueryFinishedSignature::CreateLambda(
			[WeakContext = Context.MakeWeakExecutionContext()](TSharedPtr<FEnvQueryResult> QueryResult) mutable
			{
				const FStateTreeStrongExecutionContext StrongContext = WeakContext.MakeStrongExecutionContext();

				FInstanceDataType* Data = StrongContext.GetInstanceDataPtr<FInstanceDataType>();
				if (!Data)
				{
					return;
				}

				Data->QueryRequestID = INDEX_NONE;

				const bool bSuccess = QueryResult.IsValid() && QueryResult->IsSuccessful() && QueryResult->Items.Num() > 0;
				if (bSuccess)
				{
					Data->CoverLocation = QueryResult->GetItemAsLocation(0);

					UE_VLOG_LOCATION(StrongContext.GetOwner().Get(), LogStateTree, Log,
						Data->CoverLocation, 40.0f, FColor::Green, TEXT("Cobertura encontrada"));
					UE_LOG(LogStateTree, Display, TEXT("FindCover: cobertura encontrada en %s (%d items)."),
						*Data->CoverLocation.ToString(), QueryResult->Items.Num());

					if (Data->bMoveToCover && Data->AIController)
					{
						const EPathFollowingRequestResult::Type MoveResult =
							Data->AIController->MoveToLocation(Data->CoverLocation, Data->AcceptanceRadius);

						if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
						{
							Data->bIsMoving = true;
							return;
						}

						if (MoveResult == EPathFollowingRequestResult::Failed)
						{
							UE_VLOG_UELOG(StrongContext.GetOwner().Get(), LogStateTree, Warning,
								TEXT("FindCover: no hay camino hacia la cobertura."));
							Data->ExecutionStatus = EStateTreeRunStatus::Failed;
							StrongContext.FinishTask(EStateTreeFinishTaskType::Failed);
							return;
						}
					}

					Data->ExecutionStatus = EStateTreeRunStatus::Succeeded;
				}
				else
				{
					Data->ExecutionStatus = EStateTreeRunStatus::Failed;

					UE_VLOG_UELOG(StrongContext.GetOwner().Get(), LogStateTree, Warning,
						TEXT("FindCover: el EQS no devolvio ningun punto valido."));
				}

				StrongContext.FinishTask(bSuccess ? EStateTreeFinishTaskType::Succeeded : EStateTreeFinishTaskType::Failed);
			}));

	if (InstanceData.QueryRequestID == INDEX_NONE && InstanceData.ExecutionStatus == EStateTreeRunStatus::Running)
	{
		return EStateTreeRunStatus::Failed;
	}

	return InstanceData.ExecutionStatus;
}

EStateTreeRunStatus FStateTreeTask_FindCover::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.bIsMoving && InstanceData.AIController)
	{
		if (InstanceData.AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
		{
			return EStateTreeRunStatus::Running;
		}

		InstanceData.bIsMoving = false;

		const APawn* Pawn = InstanceData.AIController->GetPawn();
		const float Tolerance = InstanceData.AcceptanceRadius + 100.0f;
		const bool bReached = Pawn && FVector::Dist2D(Pawn->GetActorLocation(), InstanceData.CoverLocation) <= Tolerance;
		InstanceData.ExecutionStatus = bReached ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
		UE_LOG(LogStateTree, Display, TEXT("FindCover: movimiento terminado, %s."), bReached ? TEXT("llego a la cobertura") : TEXT("NO llego"));
	}

	return InstanceData.ExecutionStatus;
}

void FStateTreeTask_FindCover::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.QueryRequestID != INDEX_NONE)
	{
		if (UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(Context.GetOwner()))
		{
			QueryManager->AbortQuery(InstanceData.QueryRequestID);
		}
		InstanceData.QueryRequestID = INDEX_NONE;
	}

	if (InstanceData.bIsMoving && InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
	InstanceData.bIsMoving = false;
}
