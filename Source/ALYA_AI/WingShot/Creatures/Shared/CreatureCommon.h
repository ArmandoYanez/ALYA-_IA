// Funciones de ayuda compartidas por aves y ratones.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class APawn;

// Utilidades compartidas por todas las criaturas (aves y ratones).
namespace WingShot
{
	// Eventos que reciben los StateTrees de las criaturas.
	ALYA_AI_API FGameplayTag ScaredTag();
	ALYA_AI_API FGameplayTag SpottedPlayerTag();
	ALYA_AI_API FGameplayTag StunnedTag();

	// Manda un evento al StateTree de la criatura (a traves de su AIController).
	ALYA_AI_API void SendCreatureEvent(const APawn* Creature, const FGameplayTag& Tag);

	// Aturde a las criaturas dentro del radio (impacto de la red).
	ALYA_AI_API void StunCreaturesAround(UWorld* World, const FVector& Center, float Radius, const AActor* Ignore);
}
