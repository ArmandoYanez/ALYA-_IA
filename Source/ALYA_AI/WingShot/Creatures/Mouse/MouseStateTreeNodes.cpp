// Tareas del raton. Se mueve por el NavMesh con MoveToLocation del AIController

#include "WingShot/Creatures/Mouse/MouseStateTreeNodes.h"

#include "AIController.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "WingShot/World/BirdPerch.h"
#include "WingShot/Creatures/Mouse/MouseAIController.h"
#include "WingShot/Creatures/Mouse/MouseCritter.h"

// Mouse Walk Route
EStateTreeRunStatus FStateTreeTask_MouseWalkRoute::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	const AMouseCritter* Mouse = Data.AIController ? Cast<AMouseCritter>(Data.AIController->GetPawn()) : nullptr;
	if (!Mouse)
	{
		return EStateTreeRunStatus::Failed;
	}

	// El destino es el extremo de su ruta mas lejano (ida y vuelta entre A y B)
	const AMousePathPoint* Best = nullptr;
	float BestDistance = -1.f;
	for (TActorIterator<AMousePathPoint> It(Mouse->GetWorld()); It; ++It)
	{
		if (It->RouteId != Mouse->RouteId)
		{
			continue;
		}
		const float Distance = FVector::Dist2D(It->GetActorLocation(), Mouse->GetActorLocation());
		if (Distance > BestDistance)
		{
			BestDistance = Distance;
			Best = *It;
		}
	}
	if (!Best)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.Destination = Best->GetActorLocation();
	// Camina hacia el punto usando el NavMesh
	const EPathFollowingRequestResult::Type Result = Data.AIController->MoveToLocation(Data.Destination, 40.f);
	return Result == EPathFollowingRequestResult::Failed ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_MouseWalkRoute::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}
	// Termina cuando deja de moverse (ya llego)
	return Data.AIController->GetMoveStatus() == EPathFollowingStatus::Moving ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

void FStateTreeTask_MouseWalkRoute::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AAIController* Controller = Context.GetInstanceData(*this).AIController)
	{
		// Si se interrumpe, se detiene
		Controller->StopMovement();
	}
}

// mouse Flee
EStateTreeRunStatus FStateTreeTask_MouseFlee::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	const AMouseAIController* Controller = Cast<AMouseAIController>(Data.AIController);
	AMouseCritter* Mouse = Controller ? Cast<AMouseCritter>(Controller->GetPawn()) : nullptr;
	if (!Mouse)
	{
		return EStateTreeRunStatus::Failed;
	}

	// corre en direccion contraria a la amenaza y proyecta el punto al NavMesh
	FVector Away = (Mouse->GetActorLocation() - Controller->GetThreatLocation()).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = Mouse->GetActorForwardVector();
	}
	FVector Target = Mouse->GetActorLocation() + Away * Data.FleeDistance;
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Mouse->GetWorld()))
	{
		FNavLocation Projected;
		// ajusta el punto para que quede sobre el NavMesh
		if (NavSys->ProjectPointToNavigation(Target, Projected, FVector(400.f, 400.f, 300.f)))
		{
			Target = Projected.Location;
		}
	}

	Data.PreviousSpeed = Mouse->GetCharacterMovement()->MaxWalkSpeed;
	// Corre mas rapido al huir
	Mouse->GetCharacterMovement()->MaxWalkSpeed = Data.FleeSpeed;
	const EPathFollowingRequestResult::Type Result = Data.AIController->MoveToLocation(Target, 40.f);
	return Result == EPathFollowingRequestResult::Failed ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_MouseFlee::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}
	return Data.AIController->GetMoveStatus() == EPathFollowingStatus::Moving ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

void FStateTreeTask_MouseFlee::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (AMouseCritter* Mouse = Data.AIController ? Cast<AMouseCritter>(Data.AIController->GetPawn()) : nullptr)
	{
		// regresa a su velocidad normal
		Mouse->GetCharacterMovement()->MaxWalkSpeed = Data.PreviousSpeed > 0.f ? Data.PreviousSpeed : 260.f;
	}
}
