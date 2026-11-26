#pragma once

#include "CoreMinimal.h"
#include "WingShot/Creatures/Shared/BirdBase.h"
#include "EagleBird.generated.h"

// Aguila: vuela sobre los arboles, busca presas y puede atacar al jugador.
UCLASS()
class ALYA_AI_API AEagleBird : public ABirdBase
{
	GENERATED_BODY()

public:
	AEagleBird();

	// El aguila no se puede atrapar con la red mientras ataca.
	virtual bool CanBeCaught_Implementation() const override;
};
