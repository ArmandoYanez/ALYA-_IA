#pragma once

#include "CoreMinimal.h"
#include "WingShot/Creatures/Shared/BirdBase.h"
#include "OwlBird.generated.h"

// Buho: duerme en ramas altas, caza ratones y, si lo despiertas, puede atacarte o escapar.
UCLASS()
class ALYA_AI_API AOwlBird : public ABirdBase
{
	GENERATED_BODY()

public:
	AOwlBird();
};
