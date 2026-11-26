// tipos de datos de la IA, actividades y fila de la tabla de Markov

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BirdTypes.generated.h"

// Actividades que puede elegir la cadena de Markov de una criatura
UENUM(BlueprintType)
enum class EBirdActivity : uint8
{
	Perch    UMETA(DisplayName = "Posarse"),
	Relocate UMETA(DisplayName = "Cambiar de rama"),
	Feed     UMETA(DisplayName = "Bajar a comer"),
	Sleep    UMETA(DisplayName = "Dormir"),
	Hunt     UMETA(DisplayName = "Cazar"),
	Attack   UMETA(DisplayName = "Atacar"),
	Flee     UMETA(DisplayName = "Huir"),
	Patrol   UMETA(DisplayName = "Patrullar"),
	Search   UMETA(DisplayName = "Buscar presa"),
};

// fila de la matriz de transicion, probabilidad de pasar a cada actividad
// desde la actividad actual (el nombre de la fila). Cada fila debe sumar 1
USTRUCT(BlueprintType)
struct ALYA_AI_API FBirdMarkovRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Perch = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Relocate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Feed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Sleep = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Hunt = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Attack = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Flee = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Patrol = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markov")
	float Search = 0.f;

	// Regresa la probabilidad de una actividad
	float Get(EBirdActivity Activity) const
	{
		switch (Activity)
		{
		case EBirdActivity::Perch:    return Perch;
		case EBirdActivity::Relocate: return Relocate;
		case EBirdActivity::Feed:     return Feed;
		case EBirdActivity::Sleep:    return Sleep;
		case EBirdActivity::Hunt:     return Hunt;
		case EBirdActivity::Attack:   return Attack;
		case EBirdActivity::Flee:     return Flee;
		case EBirdActivity::Patrol:   return Patrol;
		case EBirdActivity::Search:   return Search;
		}
		return 0.f;
	}

	// Cambia la probabilidad de una actividad
	void Set(EBirdActivity Activity, float Value)
	{
		switch (Activity)
		{
		case EBirdActivity::Perch:    Perch = Value; break;
		case EBirdActivity::Relocate: Relocate = Value; break;
		case EBirdActivity::Feed:     Feed = Value; break;
		case EBirdActivity::Sleep:    Sleep = Value; break;
		case EBirdActivity::Hunt:     Hunt = Value; break;
		case EBirdActivity::Attack:   Attack = Value; break;
		case EBirdActivity::Flee:     Flee = Value; break;
		case EBirdActivity::Patrol:   Patrol = Value; break;
		case EBirdActivity::Search:   Search = Value; break;
		}
	}
};
