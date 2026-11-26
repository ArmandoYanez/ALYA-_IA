#include "WingShot/World/BirdSpawner.h"

#include "EngineUtils.h"
#include "TimerManager.h"
#include "WingShot/Creatures/Shared/BirdBase.h"
#include "WingShot/World/BirdPerch.h"
#include "WingShot/Creatures/Mouse/MouseCritter.h"
#include "WingShot/Creatures/Mouse/MouseAIController.h"

ABirdSpawner::ABirdSpawner()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABirdSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Crea todas al iniciar el juego.
	for (int32 i = 0; i < Count; ++i)
	{
		SpawnCreature();
	}
}

void ABirdSpawner::SpawnCreature()
{
	if (!PawnClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* SpawnAt = nullptr;
	int32 RouteId = 0;

	// Elige un punto al azar segun el tipo.
	switch (SpawnPoint)
	{
	case ECreatureSpawnPoint::Perch:
	case ECreatureSpawnPoint::OwlPerch:
	{
		TArray<ABirdPerch*> Free;
		for (TActorIterator<ABirdPerch> It(World); It; ++It)
		{
			if (It->IsAvailableFor(nullptr) && (SpawnPoint == ECreatureSpawnPoint::Perch || It->bOwlPerch))
			{
				Free.Add(*It);
			}
		}
		SpawnAt = Free.Num() > 0 ? Free[FMath::RandRange(0, Free.Num() - 1)] : nullptr;
		break;
	}
	case ECreatureSpawnPoint::Sky:
	{
		TArray<AEagleWaypoint*> Points;
		for (TActorIterator<AEagleWaypoint> It(World); It; ++It)
		{
			Points.Add(*It);
		}
		SpawnAt = Points.Num() > 0 ? Points[FMath::RandRange(0, Points.Num() - 1)] : nullptr;
		break;
	}
	case ECreatureSpawnPoint::MouseRoute:
	{
		// Prefiere una ruta que todavia no tenga raton.
		TArray<AMousePathPoint*> Points;
		for (TActorIterator<AMousePathPoint> It(World); It; ++It)
		{
			bool bRouteTaken = false;
			for (TActorIterator<AMouseCritter> Mouse(World); Mouse; ++Mouse)
			{
				bRouteTaken |= Mouse->IsAlive() && Mouse->RouteId == It->RouteId;
			}
			if (!bRouteTaken)
			{
				Points.Add(*It);
			}
		}
		if (Points.Num() > 0)
		{
			AMousePathPoint* Point = Points[FMath::RandRange(0, Points.Num() - 1)];
			SpawnAt = Point;
			RouteId = Point->RouteId;
		}
		break;
	}
	}

	if (!SpawnAt)
	{
		return;
	}

	const FTransform Transform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), SpawnAt->GetActorLocation() + FVector(0.f, 0.f, 30.f));
	APawn* Pawn = World->SpawnActorDeferred<APawn>(PawnClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pawn)
	{
		return;
	}

	// La ruta se asigna antes de terminar el spawn para que su StateTree ya la conozca.
	if (AMouseCritter* Mouse = Cast<AMouseCritter>(Pawn))
	{
		Mouse->RouteId = RouteId;
	}
	Pawn->FinishSpawning(Transform);

	if (ABirdBase* Bird = Cast<ABirdBase>(Pawn))
	{
		if (ABirdPerch* Perch = Cast<ABirdPerch>(SpawnAt))
		{
			// Las aves empiezan posadas.
			Bird->PlaceOnPerch(Perch);
		}
	}
	// Cuando muera o la atrapen, se repone.
	Pawn->OnDestroyed.AddDynamic(this, &ABirdSpawner::HandleCreatureDestroyed);
}

void ABirdSpawner::HandleCreatureDestroyed(AActor* DestroyedActor)
{
	if (RespawnDelay <= 0.f || !GetWorld() || GetWorld()->bIsTearingDown)
	{
		return;
	}
	FTimerHandle Handle;
	// Crea otra despues de RespawnDelay segundos.
	GetWorldTimerManager().SetTimer(Handle, this, &ABirdSpawner::SpawnCreature, RespawnDelay, false);
}
