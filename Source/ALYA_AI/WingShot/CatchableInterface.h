#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CatchableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCatchableInterface : public UInterface
{
	GENERATED_BODY()
};

// Lo implementa cualquier criatura que se pueda atrapar con la pistola de red.
class ALYA_AI_API ICatchableInterface
{
	GENERATED_BODY()

public:
	// Nombre de la especie que se suma a la coleccion del jugador.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "WingShot|Catch")
	FName GetSpeciesName() const;

	// Permite rechazar la captura (por ejemplo, un aguila atacando).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "WingShot|Catch")
	bool CanBeCaught() const;

	// Se llama cuando la red atrapa a la criatura.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "WingShot|Catch")
	void OnCaughtByNet(AActor* Catcher);
};
