// nodos de Blueprint de WingShot (para usarlos desde BP_Net y otros Blueprints)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WingShotLibrary.generated.h"

UCLASS()
class ALYA_AI_API UWingShotLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Aturde a las aves y ratones dentro del radio (su StateTree pasa a Stunned)
	// ignore = la criatura que ya se atrapo (para no aturdirla)
	UFUNCTION(BlueprintCallable, Category = "WingShot", meta = (WorldContext = "WorldContextObject"))
	static void StunCreaturesAround(const UObject* WorldContextObject, FVector Location, float Radius = 300.f, AActor* Ignore = nullptr);
};
