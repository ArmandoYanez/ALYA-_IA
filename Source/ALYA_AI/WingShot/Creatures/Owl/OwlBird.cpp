// Datos del buho: su StateTree.

#include "WingShot/Creatures/Owl/OwlBird.h"

#include "StateTree.h"

AOwlBird::AOwlBird()
{
	SpeciesName = TEXT("Buho");
	// Su cerebro: ST_Owl.
	BehaviorTreeAsset = TSoftObjectPtr<UStateTree>(FSoftObjectPath(TEXT("/Game/WingShot/AI/Owl/ST_Owl.ST_Owl")));
	SitHeight = 28.f;
}
