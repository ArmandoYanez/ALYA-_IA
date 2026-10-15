// Fill out your copyright notice in the Description page of Project Settings.

#include "IA/CustomAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/Pawn.h"

ACustomAIController::ACustomAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>("StateTreeComponent");

	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerception");
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("SightConfig");

	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
	SetPerceptionComponent(*AIPerception);
}

void ACustomAIController::BeginPlay()
{
	Super::BeginPlay();

	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ACustomAIController::HandleTargetPerceptionUpdated);
}

void ACustomAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	const APawn* PerceivedPawn = Cast<APawn>(Actor);
	if (!PerceivedPawn || !PerceivedPawn->IsPlayerControlled())
	{
		return;
	}

	if (Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
	{
		return;
	}

	PerceivedPlayer = Actor;
	LastKnownPlayerLocation = Stimulus.StimulusLocation;

	if (Stimulus.WasSuccessfullySensed())
	{
		GetWorldTimerManager().ClearTimer(LoseSightTimer);
		if (!bCanSeePlayer)
		{
			bCanSeePlayer = true;
			UE_LOG(LogTemp, Display, TEXT("Perception: VE al jugador en %s"), *LastKnownPlayerLocation.ToString());
		}
	}
	else if (bCanSeePlayer && !GetWorldTimerManager().IsTimerActive(LoseSightTimer))
	{
		GetWorldTimerManager().SetTimer(LoseSightTimer, this, &ACustomAIController::ClearPlayerSight, LoseSightGraceTime, false);
	}
}

void ACustomAIController::ClearPlayerSight()
{
	bCanSeePlayer = false;
	UE_LOG(LogTemp, Display, TEXT("Perception: PERDIO al jugador en %s"), *LastKnownPlayerLocation.ToString());
}
