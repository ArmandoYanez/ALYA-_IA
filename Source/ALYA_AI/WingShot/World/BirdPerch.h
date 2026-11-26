// puntos del nivel que usan los EQS, ramas, ruta del aguila, rutas de raton y comida

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BirdPerch.generated.h"

class UBillboardComponent;

// Punto en una rama donde un ave puede posarse. Solo un ave a la vez
UCLASS()
class ALYA_AI_API ABirdPerch : public AActor
{
	GENERATED_BODY()

public:
	ABirdPerch();

	// Percha alta/escondida preferida por el buho
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perch")
	bool bOwlPerch = false;

	UFUNCTION(BlueprintPure, Category = "Perch")
	bool IsAvailableFor(const AActor* Bird) const { return !Occupant.IsValid() || Occupant.Get() == Bird; }

	// Ocupar / liberar la rama
	void SetOccupant(AActor* Bird) { Occupant = Bird; }
	void Release(const AActor* Bird) { if (Occupant.Get() == Bird) { Occupant.Reset(); } }

protected:
	// Ave que esta (o va) en la rama
	TWeakObjectPtr<AActor> Occupant;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};

// Punto de la ruta de vuelo del aguila sobre los arboles
UCLASS()
class ALYA_AI_API AEagleWaypoint : public AActor
{
	GENERATED_BODY()

public:
	AEagleWaypoint();

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};

// extremo (A o B) de la ruta que recorre un raton. Los dos puntos de una ruta comparten RouteId
UCLASS()
class ALYA_AI_API AMousePathPoint : public AActor
{
	GENERATED_BODY()

public:
	AMousePathPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mouse")
	int32 RouteId = 0;

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};

// zona del suelo donde las aves bajan a comer
UCLASS()
class ALYA_AI_API ABirdFeedSpot : public AActor
{
	GENERATED_BODY()

public:
	ABirdFeedSpot();

	// Radio dentro del cual el ave elige un punto al azar para picotear
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feed")
	float Radius = 120.f;

	FVector GetRandomFeedLocation() const;

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};
