// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Components/StateTreeComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "CustomAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/*
Alojar componente de los state trees
*/
UCLASS()
class ALYA_AI_API ACustomAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACustomAIController();

	// Objetivo percibido (el jugador). Lo leen el Evaluator y el contexto EQS EQC_PlayerTarget.
	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	AActor* GetTargetActor() const { return TargetActor; }

	// True mientras el NPC tiene al objetivo a la vista.
	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	bool HasTarget() const { return bHasTarget; }

protected:
	virtual void BeginPlay() override;

	// Se ejecuta cuando el jugador entra o sale del cono de vision.
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// Se ejecuta cuando termina el tiempo de gracia sin volver a ver al jugador.
	void ClearPlayerSight();

	// Componente que ejecuta el StateTree.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;

	// Componente de percepcion (los "sentidos" del NPC).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	// Configuracion del sentido de vista.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	// Segundos que espera antes de dar al jugador por perdido (evita que el NPC tiemble).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LoseSightGraceTime = 1.0f;

	FTimerHandle LoseSightTimer;

	// Ultimo objetivo percibido; se actualiza solo en OnTargetPerceptionUpdated.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<AActor> TargetActor = nullptr;

	// True mientras el NPC ve al objetivo.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	bool bHasTarget = false;
};
