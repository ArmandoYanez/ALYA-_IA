#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WingShotHUD.generated.h"

// HUD de prototipo dibujado con Canvas: mira, coleccion, sigilo y vista de binoculares.
UCLASS()
class ALYA_AI_API AWingShotHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
