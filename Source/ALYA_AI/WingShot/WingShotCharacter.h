#pragma once

#include "CoreMinimal.h"
#include "ALYA_AICharacter.h"
#include "WingShotCharacter.generated.h"

class ANetProjectile;
class UInputAction;

// Jugador de WingShot: sigilo, binoculares, pistola de red y coleccion de aves.
UCLASS()
class ALYA_AI_API AWingShotCharacter : public AALYA_AICharacter
{
	GENERATED_BODY()

public:
	AWingShotCharacter();

	virtual void Tick(float DeltaSeconds) override;

	// Dispara la red (tambien se puede llamar desde Blueprint o pruebas).
	UFUNCTION(BlueprintCallable, Category = "WingShot|Net")
	void FireNet();

	// La red llama esto al atrapar a una criatura.
	UFUNCTION(BlueprintCallable, Category = "WingShot|Collection")
	void RegisterCatch(FName Species);

	UFUNCTION(BlueprintPure, Category = "WingShot|Movement")
	bool IsSneaking() const { return bIsSneaking; }

	UFUNCTION(BlueprintPure, Category = "WingShot|Binoculars")
	bool IsUsingBinoculars() const { return bUsingBinoculars; }

	UFUNCTION(BlueprintPure, Category = "WingShot|Collection")
	int32 GetTotalCaught() const { return TotalCaught; }

	const TMap<FName, int32>& GetCaughtBySpecies() const { return CaughtBySpecies; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void DoAim(float Yaw, float Pitch) override;

	void StartSneak();
	void StopSneak();
	void StartBinoculars();
	void StopBinoculars();

	// Emite un ruido de pasos que AI Perception (Hearing) puede escuchar.
	void UpdateFootstepNoise(float DeltaSeconds);

	// --- Input ---
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SneakAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> BinocularsAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireNetAction;

	// --- Movimiento y sigilo ---
	UPROPERTY(EditAnywhere, Category = "WingShot|Movement")
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, Category = "WingShot|Movement")
	float SneakSpeed = 200.0f;

	// Cada cuantos segundos se emite un paso mientras se camina.
	UPROPERTY(EditAnywhere, Category = "WingShot|Noise")
	float FootstepInterval = 0.45f;

	// Distancia a la que las aves escuchan los pasos normales / sigilosos.
	UPROPERTY(EditAnywhere, Category = "WingShot|Noise")
	float WalkNoiseRange = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "WingShot|Noise")
	float SneakNoiseRange = 250.0f;

	// --- Binoculares ---
	UPROPERTY(EditAnywhere, Category = "WingShot|Binoculars")
	float BinocularsFOV = 20.0f;

	UPROPERTY(EditAnywhere, Category = "WingShot|Binoculars")
	float ZoomInterpSpeed = 10.0f;

	// Multiplicador de sensibilidad de la camara mientras se usan los binoculares.
	UPROPERTY(EditAnywhere, Category = "WingShot|Binoculars")
	float BinocularsLookScale = 0.25f;

	// --- Pistola de red ---
	UPROPERTY(EditAnywhere, Category = "WingShot|Net")
	TSubclassOf<ANetProjectile> NetClass;

	UPROPERTY(EditAnywhere, Category = "WingShot|Net")
	float NetCooldown = 0.8f;

	// --- Estado ---
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "WingShot|State")
	bool bIsSneaking = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "WingShot|State")
	bool bUsingBinoculars = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "WingShot|Collection")
	TMap<FName, int32> CaughtBySpecies;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "WingShot|Collection")
	int32 TotalCaught = 0;

private:
	float DefaultFOV = 90.0f;
	float FootstepTimer = 0.0f;
	float LastNetTime = -1000.0f;
};
