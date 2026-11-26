// clase base de todas las aves (ave comun, buho y aguila)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WingShot/Player/CatchableInterface.h"
#include "BirdBase.generated.h"

class ABirdPerch;
class USphereComponent;
class UStateTree;

// Ave base de WingShot, solo logica (vuelo en arco, perchas, comer en el suelo, ser atrapada)
// no tiene modelo, animacion ni texto, todo lo visual se arma en el Blueprint hijo
UCLASS(Abstract, Blueprintable)
class ALYA_AI_API ABirdBase : public APawn, public ICatchableInterface
{
	GENERATED_BODY()

public:
	ABirdBase();

	virtual void Tick(float DeltaSeconds) override;

	// --- Vuelo ---
	// Empieza a volar hacia un punto
	void FlyTo(const FVector& Destination, float Speed, float ArcHeight);
	void StopFlight() { bFlying = false; }

	// Mueve el destino del vuelo actual (para perseguir a una presa que camina)
	void UpdateFlightTarget(const FVector& Destination);

	// llamado al llegar a un destino, se posa en la percha, mata a la presa o se queda en el suelo
	void OnArrived(AActor* Target);

	// --- Sueno (buho) ---
	void SetSleeping(bool bInSleeping) { bSleeping = bInSleeping; }

	// --- Aturdido (reaccion a dano/stun) ---
	void SetStunned(bool bInStunned) { bStunned = bInStunned; if (bStunned) { StopFlight(); } }

	// --- Estado del ave, se leen desde el Blueprint para animar tu modelo ---
	UFUNCTION(BlueprintPure, Category = "Bird|State")
	bool IsFlying() const { return bFlying; }

	UFUNCTION(BlueprintPure, Category = "Bird|State")
	bool IsSleeping() const { return bSleeping; }

	UFUNCTION(BlueprintPure, Category = "Bird|State")
	bool IsStunned() const { return bStunned; }

	UFUNCTION(BlueprintPure, Category = "Bird|State")
	bool IsOnGround() const { return bOnGround; }

	// --- Estado del StateTree (para el texto sobre la cabeza) ---
	// Lo llama el evaluador StateLabel cuando cambia el estado activo
	void SetCurrentState(FName NewState);

	// Nombre del estado actual del StateTree (ej. "Perch", "FlyToBranch")
	UFUNCTION(BlueprintPure, Category = "Bird|State")
	FName GetCurrentState() const { return CurrentState; }

	// Evento para el Blueprint, se dispara cada vez que cambia el estado. Ahi actualizas tu texto
	UFUNCTION(BlueprintImplementableEvent, Category = "Bird|State")
	void OnStateChanged(FName NewState);

	// Aparta una percha para que otra ave no la elija mientras esta vuela hacia ella
	void ReservePerch(ABirdPerch* Perch);
	// coloca al ave sobre la rama
	void PlaceOnPerch(ABirdPerch* Perch);
	ABirdPerch* GetCurrentPerch() const { return CurrentPerch.Get(); }

	// StateTree de la especie (lo usa el controller)
	UStateTree* GetBehaviorStateTree() const;

	// --- ICatchableInterface ---
	virtual FName GetSpeciesName_Implementation() const override { return SpeciesName; }
	virtual bool CanBeCaught_Implementation() const override { return !bCaught; }
	virtual void OnCaughtByNet_Implementation(AActor* Catcher) override;

protected:
	// libera las ramas ocupadas o apartadas
	void ReleasePerches();

	// --- Datos de especie ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bird")
	FName SpeciesName = TEXT("Ave");

	UPROPERTY(EditAnywhere, Category = "Bird")
	// StateTree que usa esta especie
	TSoftObjectPtr<UStateTree> BehaviorTreeAsset;

	// Altura de la percha/suelo al centro del ave
	UPROPERTY(EditAnywhere, Category = "Bird")
	float SitHeight = 18.f;

	// --- Componentes ---
	// Raiz y colision del ave (la red tiene que tocarla). Tu modelo va como hijo de esta esfera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

private:
	// Estado interno del ave
	bool bFlying = false;
	bool bOnGround = false;
	bool bCaught = false;
	bool bSleeping = false;
	bool bStunned = false;
	FName CurrentState;
	// datos del vuelo actual
	FVector FlightStart = FVector::ZeroVector;
	FVector FlightEnd = FVector::ZeroVector;
	float FlightAlpha = 0.f;
	float FlightDuration = 1.f;
	float FlightArc = 0.f;

	TWeakObjectPtr<ABirdPerch> CurrentPerch;
	TWeakObjectPtr<ABirdPerch> ReservedPerch;
};
