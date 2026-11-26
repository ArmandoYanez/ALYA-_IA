// Spawner de criaturas (se colocan 4 en el nivel: aves comunes, buhos, aguila y ratones).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BirdSpawner.generated.h"

UENUM(BlueprintType)
// Tipo de lugar donde aparece la criatura.
enum class ECreatureSpawnPoint : uint8
{
	Perch      UMETA(DisplayName = "Percha libre"),
	OwlPerch   UMETA(DisplayName = "Percha de buho"),
	Sky        UMETA(DisplayName = "Cielo (ruta del aguila)"),
	MouseRoute UMETA(DisplayName = "Ruta de raton"),
};

// Coloca criaturas al iniciar y repone las que el jugador atrapa o los depredadores cazan.
UCLASS()
class ALYA_AI_API ABirdSpawner : public AActor
{
	GENERATED_BODY()

public:
	ABirdSpawner();

	UPROPERTY(EditAnywhere, Category = "Spawner")
	// Que criatura crear.
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	// Donde aparece.
	ECreatureSpawnPoint SpawnPoint = ECreatureSpawnPoint::Perch;

	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0"))
	// Cuantas crear al iniciar.
	int32 Count = 10;

	// Segundos para reponer una criatura (0 = no reponer).
	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0.0"))
	float RespawnDelay = 25.f;

protected:
	virtual void BeginPlay() override;

	void SpawnCreature();

	UFUNCTION()
	void HandleCreatureDestroyed(AActor* DestroyedActor);
};
