#include "WingShot/WingShotHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "WingShot/WingShotCharacter.h"

void AWingShotHUD::DrawHUD()
{
	Super::DrawHUD();

	const AWingShotCharacter* Player = Cast<AWingShotCharacter>(GetOwningPawn());
	if (!Player || !Canvas)
	{
		return;
	}

	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const FVector2D Center(W * 0.5f, H * 0.5f);
	UFont* Font = GEngine->GetMediumFont();

	if (Player->IsUsingBinoculars())
	{
		// Dos circulos de lente: se oscurece todo lo que queda fuera.
		const float Radius = H * 0.36f;
		const FLinearColor Black(0.f, 0.f, 0.f, 1.f);
		const float LensOffset = Radius * 0.75f;
		DrawRect(Black, 0.f, 0.f, Center.X - LensOffset - Radius, H);
		DrawRect(Black, Center.X + LensOffset + Radius, 0.f, W, H);
		DrawRect(Black, 0.f, 0.f, W, Center.Y - Radius);
		DrawRect(Black, 0.f, Center.Y + Radius, W, H);
		DrawText(TEXT("BINOCULARES"), FLinearColor::White, Center.X - 60.f, Center.Y + Radius + 10.f, Font);
	}

	// Mira de la pistola de red.
	const FLinearColor CrossColor(1.f, 1.f, 1.f, 0.8f);
	DrawLine(Center.X - 10.f, Center.Y, Center.X + 10.f, Center.Y, CrossColor, 1.5f);
	DrawLine(Center.X, Center.Y - 10.f, Center.X, Center.Y + 10.f, CrossColor, 1.5f);

	// Coleccion de aves.
	float Y = 30.f;
	DrawText(FString::Printf(TEXT("Aves atrapadas: %d"), Player->GetTotalCaught()), FLinearColor::White, 30.f, Y, Font, 1.2f);
	for (const TPair<FName, int32>& Entry : Player->GetCaughtBySpecies())
	{
		Y += 26.f;
		DrawText(FString::Printf(TEXT("  %s x%d"), *Entry.Key.ToString(), Entry.Value), FLinearColor(0.8f, 1.f, 0.8f), 30.f, Y, Font);
	}

	if (Player->IsSneaking())
	{
		DrawText(TEXT("SIGILO"), FLinearColor(0.6f, 0.8f, 1.f), 30.f, H - 60.f, Font, 1.3f);
	}

	DrawText(TEXT("Ctrl: sigilo   Clic der.: binoculares   Clic izq.: red"), FLinearColor(1.f, 1.f, 1.f, 0.6f), W - 520.f, H - 40.f, Font);
}
