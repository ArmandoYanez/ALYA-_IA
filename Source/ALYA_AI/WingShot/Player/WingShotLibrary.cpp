#include "WingShot/Player/WingShotLibrary.h"

#include "Engine/Engine.h"
#include "WingShot/Creatures/Shared/CreatureCommon.h"

void UWingShotLibrary::StunCreaturesAround(const UObject* WorldContextObject, FVector Location, float Radius, AActor* Ignore)
{
	// Usa la misma funcion que comparten las criaturas.
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	WingShot::StunCreaturesAround(World, Location, Radius, Ignore);
}
