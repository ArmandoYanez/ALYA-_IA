// Fill out your copyright notice in the Description page of Project Settings.

#include "StateTreeTask_FindCover.h"

#include "StateTreeExecutionContext.h"           
#include "StateTreeAsyncExecutionContext.h"      
#include "EnvironmentQuery/EnvQuery.h"          
#include "EnvironmentQuery/EnvQueryManager.h"   
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "VisualLogger/VisualLogger.h"          

namespace
{
	/**
	 * Resuelve el AIController a partir del Owner del contexto.
	 *
	 * El Owner depende de donde viva el StateTreeComponent:
	 *  - Componente en el AIController  -> Owner es el AIController (nuestro caso).
	 *  - Componente en el Pawn          -> Owner es el Pawn, y sacamos su Controller.
	 * Cubrimos ambos casos para que la tarea no se rompa si mueven el componente de sitio.
	 */
	AAIController* ResolveAIController(UObject* Owner)
	{
		if (AAIController* AsController = Cast<AAIController>(Owner))
		{
			return AsController;
		}

		if (const APawn* AsPawn = Cast<APawn>(Owner))
		{
			return Cast<AAIController>(AsPawn->GetController());
		}

		// Por si el Owner llegara a ser el propio componente en lugar del actor.
		if (const UActorComponent* AsComponent = Cast<UActorComponent>(Owner))
		{
			return ResolveAIController(AsComponent->GetOwner());
		}

		return nullptr;
	}
}

EStateTreeRunStatus FStateTreeTask_FindCover::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 1. Memoria mutable de ESTE agente. La struct de la tarea es compartida y const (patron Flyweight),
	//    asi que todo estado vivo se guarda aqui.
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// 2. Arrancamos en Running mientras el EQS resuelve en segundo plano.
	InstanceData.ExecutionStatus = EStateTreeRunStatus::Running;
	InstanceData.QueryRequestID = INDEX_NONE;

	// 3. Si nadie enlazo (bind) el AIController en el editor, lo deducimos del Owner.
	if (!InstanceData.AIController)
	{
		InstanceData.AIController = ResolveAIController(Context.GetOwner());
	}

	// 4. Validaciones: sin asset de EQS o sin controller no hay nada que hacer.
	if (!InstanceData.CoverQuery || !InstanceData.AIController)
	{
		UE_VLOG(Context.GetOwner(), LogStateTree, Error,
			TEXT("FindCover: falta CoverQuery (%s) o AIController (%s)."),
			*GetNameSafe(InstanceData.CoverQuery), *GetNameSafe(InstanceData.AIController));
		return EStateTreeRunStatus::Failed;
	}

	// 5. El Pawn es el "Querier": el centro desde el que el EQS genera y puntua los puntos.
	APawn* ControlledPawn = InstanceData.AIController->GetPawn();
	if (!ControlledPawn)
	{
		UE_VLOG(Context.GetOwner(), LogStateTree, Error, TEXT("FindCover: el AIController no posee ningun Pawn."));
		return EStateTreeRunStatus::Failed;
	}

	// 6. Peticion al EQS: asset + duenio de la consulta.
	FEnvQueryRequest QueryRequest(InstanceData.CoverQuery, ControlledPawn);

	// 7. Lanzamos la consulta asincrona.
	InstanceData.QueryRequestID = QueryRequest.Execute(
		InstanceData.RunMode,
		FQueryFinishedSignature::CreateLambda(
			[WeakContext = Context.MakeWeakExecutionContext()](TSharedPtr<FEnvQueryResult> QueryResult) mutable
			{
				// Debe ser un lvalue con nombre: GetInstanceDataPtr solo existe para lvalues.
				const FStateTreeStrongExecutionContext StrongContext = WeakContext.MakeStrongExecutionContext();

				FInstanceDataType* Data = StrongContext.GetInstanceDataPtr<FInstanceDataType>();
				if (!Data)
				{
					// El estado ya salio o el arbol murio: la consulta llego tarde, se descarta sin riesgo.
					return;
				}

				// La peticion ya termino, dejamos de considerarla pendiente para ExitState.
				Data->QueryRequestID = INDEX_NONE;

				const bool bSuccess = QueryResult.IsValid() && QueryResult->IsSuccessful() && QueryResult->Items.Num() > 0;
				if (bSuccess)
				{
					// Item 0 = el mejor punto segun los tests del EQS (o uno aleatorio segun el RunMode).
					Data->CoverLocation = QueryResult->GetItemAsLocation(0);
					Data->ExecutionStatus = EStateTreeRunStatus::Succeeded;

					UE_VLOG_LOCATION(StrongContext.GetOwner().Get(), LogStateTree, Log,
						Data->CoverLocation, 40.0f, FColor::Green, TEXT("Cobertura encontrada"));
				}
				else
				{
					// Ningun punto paso los tests (sin NavMesh, todo a la vista del jugador, etc.).
					Data->ExecutionStatus = EStateTreeRunStatus::Failed;

					UE_VLOG(StrongContext.GetOwner().Get(), LogStateTree, Warning,
						TEXT("FindCover: el EQS no devolvio ningun punto valido."));
				}

				// Desbloquea la transicion aunque la tarea no estuviera ticando.
				StrongContext.FinishTask(bSuccess ? EStateTreeFinishTaskType::Succeeded : EStateTreeFinishTaskType::Failed);
			}));

	// Si Execute no devolvio un id valido, la consulta ni siquiera arranco.
	// (Ojo: si el EQS resolvio de forma sincrona, el callback ya corrio y dejo ExecutionStatus listo.)
	if (InstanceData.QueryRequestID == INDEX_NONE && InstanceData.ExecutionStatus == EStateTreeRunStatus::Running)
	{
		return EStateTreeRunStatus::Failed;
	}

	return InstanceData.ExecutionStatus;
}

EStateTreeRunStatus FStateTreeTask_FindCover::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// Cada frame StateTree pregunta como va la tarea:
	//  - Running   -> el EQS sigue evaluando.
	//  - Succeeded -> ya hay CoverLocation lista para enlazar al Target de un MoveTo.
	//  - Failed    -> no hubo cobertura viable.
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.ExecutionStatus;
}

void FStateTreeTask_FindCover::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// Al salir del estado cancelamos la consulta si seguia en vuelo, para no dejar
	// trabajo huerfano ocupando el EnvQueryManager.
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.QueryRequestID != INDEX_NONE)
	{
		if (UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(Context.GetOwner()))
		{
			QueryManager->AbortQuery(InstanceData.QueryRequestID);
		}
		InstanceData.QueryRequestID = INDEX_NONE;
	}
}
