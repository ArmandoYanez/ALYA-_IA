#include "StateTreeCondition_PlayerNear.h"

#include "StateTreeExecutionContext.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

bool FStateTreeCondition_PlayerNear::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const APawn* Pawn = InstanceData.Pawn;
	const APawn* Player = Pawn ? UGameplayStatics::GetPlayerPawn(Pawn, 0) : nullptr;

	bool bIsNear = false;
	if (Pawn && Player)
	{
		bIsNear = FVector::DistSquared(Pawn->GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(InstanceData.Distance);

		if (bIsNear && InstanceData.bRequireLineOfSight)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerNearLOS), false, Pawn);
			Params.AddIgnoredActor(Player);

			FHitResult Hit;
			const bool bBlocked = Pawn->GetWorld()->LineTraceSingleByChannel(
				Hit, Pawn->GetPawnViewLocation(), Player->GetPawnViewLocation(), ECC_Visibility, Params);
			bIsNear = !bBlocked;
		}
	}

	return InstanceData.bInvert ? !bIsNear : bIsNear;
}
