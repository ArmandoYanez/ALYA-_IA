#pragma once

#include "CoreMinimal.h"
#include "WingShot/Creatures/Shared/BirdBase.h"
#include "CommonBird.generated.h"

// ave comun, la mas abundante. No busca problemas, vuela entre ramas, baja a comer
// y huye cuando escucha pasos o ve al jugador demasiado cerca
UCLASS()
class ALYA_AI_API ACommonBird : public ABirdBase
{
	GENERATED_BODY()

public:
	ACommonBird();
};
