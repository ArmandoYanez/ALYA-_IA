#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Components/ActorComponent.h"

namespace ALYA::StateTree
{
	inline AAIController* ResolveAIController(UObject* Owner)
	{
		if (AAIController* AsController = Cast<AAIController>(Owner))
		{
			return AsController;
		}

		if (const APawn* AsPawn = Cast<APawn>(Owner))
		{
			return Cast<AAIController>(AsPawn->GetController());
		}

		if (const UActorComponent* AsComponent = Cast<UActorComponent>(Owner))
		{
			return ResolveAIController(AsComponent->GetOwner());
		}

		return nullptr;
	}
}
