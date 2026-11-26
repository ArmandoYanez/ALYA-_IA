// raton (Character para usar el NavMesh)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MouseCritter.generated.h"

class UStateTree;

// raton, presa del buho y del aguila. Camina de A a B por su ruta y muere al ser cazado
// Solo logica, el modelo, la animacion y el texto se arman en el Blueprint hijo (BP_Mouse)
UCLASS()
class ALYA_AI_API AMouseCritter : public ACharacter
{
	GENERATED_BODY()

public:
	AMouseCritter();

	UFUNCTION(BlueprintPure, Category = "Mouse")
	bool IsAlive() const { return !bDead; }

	// Lo llama el depredador al atraparlo, se queda estatico y desaparece
	void Kill(AActor* Killer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mouse")
	// ruta que recorre (la asigna el spawner)
	int32 RouteId = 0;

	UStateTree* GetBehaviorStateTree() const;

	void SetStunned(bool bInStunned) { bStunned = bInStunned; }

	UFUNCTION(BlueprintPure, Category = "Mouse")
	bool IsStunned() const { return bStunned; }

	// --- Estado del StateTree (para el texto sobre la cabeza) ---
	// Lo llama el evaluador StateLabel cuando cambia el estado activo
	void SetCurrentState(FName NewState);

	// nombre del estado actual del StateTree (ej. "Walk", "Flee")
	UFUNCTION(BlueprintPure, Category = "Mouse")
	FName GetCurrentState() const { return CurrentState; }

	// Evento para el Blueprint, se dispara cada vez que cambia el estado. Ahi actualizas tu texto
	UFUNCTION(BlueprintImplementableEvent, Category = "Mouse")
	void OnStateChanged(FName NewState);

	// evento para el Blueprint, lo cazaron (ahi puedes poner su animacion o efecto de muerte)
	UFUNCTION(BlueprintImplementableEvent, Category = "Mouse")
	void OnKilled();

protected:
	UPROPERTY(EditAnywhere, Category = "Mouse")
	// su cerebro, ST_Mouse
	TSoftObjectPtr<UStateTree> BehaviorTreeAsset;

private:
	bool bDead = false;
	bool bStunned = false;
	FName CurrentState;
};
