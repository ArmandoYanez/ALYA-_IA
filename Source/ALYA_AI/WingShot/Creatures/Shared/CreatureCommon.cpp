#include "WingShot/Creatures/Shared/CreatureCommon.h"

#include "AIController.h"
#include "Components/StateTreeComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "WingShot/Creatures/Shared/BirdBase.h"
#include "WingShot/Creatures/Mouse/MouseCritter.h"
#include "WingShot/Creatures/Mouse/MouseAIController.h"

namespace WingShot
{
	// Tags de los eventos (definidos en Config/DefaultGameplayTags.ini).
	FGameplayTag ScaredTag()
	{
		static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("WingShot.Bird.Scared"));
		return Tag;
	}

	FGameplayTag SpottedPlayerTag()
	{
		static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("WingShot.Bird.SpottedPlayer"));
		return Tag;
	}

	FGameplayTag StunnedTag()
	{
		static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("WingShot.Creature.Stunned"));
		return Tag;
	}

	void SendCreatureEvent(const APawn* Creature, const FGameplayTag& Tag)
	{
		const AController* Controller = Creature ? Creature->GetController() : nullptr;
		if (UStateTreeComponent* StateTree = Controller ? Controller->FindComponentByClass<UStateTreeComponent>() : nullptr)
		{
			// El StateTree vive en el controller, no en el pawn.
			StateTree->SendStateTreeEvent(Tag);
		}
	}

	void StunCreaturesAround(UWorld* World, const FVector& Center, float Radius, const AActor* Ignore)
	{
		if (!World)
		{
			return;
		}
		// Busca aves y ratones dentro del radio y les manda Stunned.
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Pawn = *It;
			const bool bCreature = Pawn->IsA<ABirdBase>() || Pawn->IsA<AMouseCritter>();
			if (bCreature && Pawn != Ignore && FVector::Dist(Pawn->GetActorLocation(), Center) <= Radius)
			{
				SendCreatureEvent(Pawn, StunnedTag());
			}
		}
	}
}
