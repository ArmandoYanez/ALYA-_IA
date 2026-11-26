// Constructores de los puntos del nivel (solo un icono en el editor).

#include "WingShot/World/BirdPerch.h"

#include "Components/BillboardComponent.h"

ABirdPerch::ABirdPerch()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(RootComponent);
	}
#endif
}

AEagleWaypoint::AEagleWaypoint()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(RootComponent);
	}
#endif
}

AMousePathPoint::AMousePathPoint()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(RootComponent);
	}
#endif
}

ABirdFeedSpot::ABirdFeedSpot()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(RootComponent);
	}
#endif
}

FVector ABirdFeedSpot::GetRandomFeedLocation() const
{
	// Punto al azar dentro del circulo.
	const FVector2D Offset = FMath::RandPointInCircle(Radius);
	return GetActorLocation() + FVector(Offset.X, Offset.Y, 0.f);
}
