#include "WingShot/Creatures/Shared/BirdBase.h"

#include "AIController.h"
#include "Components/SphereComponent.h"
#include "BrainComponent.h"
#include "StateTree.h"
#include "WingShot/Creatures/Shared/BirdAIController.h"
#include "WingShot/World/BirdPerch.h"
#include "WingShot/Creatures/Mouse/MouseCritter.h"

ABirdBase::ABirdBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// Cada ave se controla con un BirdAIController
	AIControllerClass = ABirdAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// colision, la red choca con esta esfera
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(28.f);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = Collision;
}

void ABirdBase::SetCurrentState(FName NewState)
{
	CurrentState = NewState;
	// Avisa al Blueprint para que actualice su texto
	OnStateChanged(NewState);
}

UStateTree* ABirdBase::GetBehaviorStateTree() const
{
	return BehaviorTreeAsset.LoadSynchronous();
}

void ABirdBase::FlyTo(const FVector& Destination, float Speed, float ArcHeight)
{
	// Guarda inicio y fin; el Tick mueve al ave entre ellos
	FlightStart = GetActorLocation();
	FlightEnd = Destination + FVector(0.f, 0.f, SitHeight);
	FlightAlpha = 0.f;
	FlightDuration = FMath::Max(0.2f, FVector::Dist(FlightStart, FlightEnd) / FMath::Max(Speed, 1.f));
	FlightArc = ArcHeight;
	bFlying = true;
	bOnGround = false;

	// Al despegar deja libre la rama donde estaba
	if (CurrentPerch.IsValid() && CurrentPerch != ReservedPerch)
	{
		CurrentPerch->Release(this);
		CurrentPerch.Reset();
	}
}

void ABirdBase::UpdateFlightTarget(const FVector& Destination)
{
	if (bFlying)
	{
		FlightEnd = Destination + FVector(0.f, 0.f, SitHeight);
	}
}

void ABirdBase::OnArrived(AActor* Target)
{
	bFlying = false;
	if (ABirdPerch* Perch = Cast<ABirdPerch>(Target))
	{
		PlaceOnPerch(Perch);
	}
	else if (AMouseCritter* Mouse = Cast<AMouseCritter>(Target))
	{
		// caza, si la presa sigue cerca, la atrapa y se queda en el suelo comiendo
		if (Mouse->IsAlive() && FVector::Dist2D(Mouse->GetActorLocation(), GetActorLocation()) < 250.f)
		{
			Mouse->Kill(this);
		}
		bOnGround = true;
	}
	else
	{
		// llego al suelo (zona de comida)
		bOnGround = true;
	}
}

// aparta la rama a la que va a volar
void ABirdBase::ReservePerch(ABirdPerch* Perch)
{
	if (ReservedPerch.IsValid() && ReservedPerch != CurrentPerch)
	{
		ReservedPerch->Release(this);
	}
	ReservedPerch = Perch;
	if (Perch)
	{
		Perch->SetOccupant(this);
	}
}

void ABirdBase::PlaceOnPerch(ABirdPerch* Perch)
{
	if (CurrentPerch.IsValid() && CurrentPerch != Perch)
	{
		CurrentPerch->Release(this);
	}
	CurrentPerch = Perch;
	ReservedPerch = Perch;
	bOnGround = false;
	if (Perch)
	{
		Perch->SetOccupant(this);
		SetActorLocation(Perch->GetActorLocation() + FVector(0.f, 0.f, SitHeight));
	}
}

void ABirdBase::ReleasePerches()
{
	if (CurrentPerch.IsValid())
	{
		CurrentPerch->Release(this);
	}
	if (ReservedPerch.IsValid())
	{
		ReservedPerch->Release(this);
	}
	CurrentPerch.Reset();
	ReservedPerch.Reset();
}

// Tick solo para mover al ave en su vuelo. La IA no esta aqui, esta en el StateTree
void ABirdBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bStunned || !bFlying)
	{
		return;
	}

	// Vuelo en arco, interpolacion lineal + parabola de altura
	FlightAlpha = FMath::Min(1.f, FlightAlpha + DeltaSeconds / FlightDuration);
	const float Smooth = FMath::InterpEaseInOut(0.f, 1.f, FlightAlpha, 1.6f);
	FVector NewLocation = FMath::Lerp(FlightStart, FlightEnd, Smooth);
	NewLocation.Z += FlightArc * 4.f * Smooth * (1.f - Smooth);

	// gira hacia donde vuela
	const FVector Direction = (FlightEnd - FlightStart).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		const FRotator TargetRotation(0.f, Direction.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, 8.f));
	}
	SetActorLocation(NewLocation);

	if (FlightAlpha >= 1.f)
	{
		bFlying = false;
	}
}

// atrapada por la red, apaga la IA y desaparece
void ABirdBase::OnCaughtByNet_Implementation(AActor* Catcher)
{
	if (bCaught)
	{
		return;
	}
	bCaught = true;
	bFlying = false;
	ReleasePerches();

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Caught"));
		}
		AIController->UnPossess();
		AIController->Destroy();
	}

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetLifeSpan(0.2f);
}
