// Datos del ave comun, su StateTree

#include "WingShot/Creatures/CommonBird/CommonBird.h"

#include "StateTree.h"

ACommonBird::ACommonBird()
{
	SpeciesName = TEXT("Ave comun");
	// Su cerebro, ST_CommonBird
	BehaviorTreeAsset = TSoftObjectPtr<UStateTree>(FSoftObjectPath(TEXT("/Game/WingShot/AI/CommonBird/ST_CommonBird.ST_CommonBird")));
	SitHeight = 24.f;
}
