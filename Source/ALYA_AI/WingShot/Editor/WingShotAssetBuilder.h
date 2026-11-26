#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WingShotAssetBuilder.generated.h"

// Genera por codigo los assets de IA de WingShot (tablas de Markov, EQS y StateTrees).
// Solo funciona en el editor. Se llama desde Python:
//   unreal.WingShotAssetBuilder.build_all_assets()
UCLASS()
class ALYA_AI_API UWingShotAssetBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Crea/actualiza los assets de todas las criaturas. Devuelve un resumen.
	UFUNCTION(BlueprintCallable, Category = "WingShot|Builder")
	static FString BuildAllAssets();

	UFUNCTION(BlueprintCallable, Category = "WingShot|Builder")
	static FString BuildCommonBirdAssets();

	UFUNCTION(BlueprintCallable, Category = "WingShot|Builder")
	static FString BuildOwlAssets();

	UFUNCTION(BlueprintCallable, Category = "WingShot|Builder")
	static FString BuildEagleAssets();

	UFUNCTION(BlueprintCallable, Category = "WingShot|Builder")
	static FString BuildMouseAssets();
};
