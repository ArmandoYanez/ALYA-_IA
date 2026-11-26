// contextos y tests de EQS hechos a medida para las aves

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "BirdEQS.generated.h"

// Contexto EQS, el jugador (primer pawn controlado por jugador)
UCLASS(meta = (DisplayName = "EQC_WingShotPlayer"))
class ALYA_AI_API UEnvQueryContext_WingShotPlayer : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

// Contexto EQS, la ultima ubicacion de la amenaza que percibio el ave (desde su AIController)
UCLASS(meta = (DisplayName = "EQC_BirdThreat"))
class ALYA_AI_API UEnvQueryContext_BirdThreat : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

// Test EQS, descarta las perchas que ya tiene ocupadas (o apartadas) otra ave
UCLASS(meta = (DisplayName = "Perch Available"))
class ALYA_AI_API UEnvQueryTest_PerchAvailable : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_PerchAvailable();

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

// Test EQS, descarta presas (ratones) que ya estan muertas
UCLASS(meta = (DisplayName = "Prey Alive"))
class ALYA_AI_API UEnvQueryTest_PreyAlive : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_PreyAlive();

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

// Test EQS, puntua mejor las perchas marcadas como de buho (altas y escondidas)
UCLASS(meta = (DisplayName = "Owl Perch"))
class ALYA_AI_API UEnvQueryTest_OwlPerch : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_OwlPerch();

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
