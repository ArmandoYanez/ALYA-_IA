// Datos del aguila: su StateTree.

#include "WingShot/Creatures/Eagle/EagleBird.h"

#include "StateTree.h"
#include "WingShot/Creatures/Eagle/EagleAIController.h"

AEagleBird::AEagleBird()
{
	SpeciesName = TEXT("Aguila");
	// Usa su propio controller (ve mas lejos).
	AIControllerClass = AEagleAIController::StaticClass();
	// Su cerebro: ST_Eagle.
	BehaviorTreeAsset = TSoftObjectPtr<UStateTree>(FSoftObjectPath(TEXT("/Game/WingShot/AI/Eagle/ST_Eagle.ST_Eagle")));
	SitHeight = 40.f;
}

bool AEagleBird::CanBeCaught_Implementation() const
{
	const AEagleAIController* EagleController = Cast<AEagleAIController>(GetController());
	return Super::CanBeCaught_Implementation() && !(EagleController && EagleController->IsAttacking());
}
