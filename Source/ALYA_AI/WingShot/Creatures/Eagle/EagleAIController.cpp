// Controller del aguila: vista larga y avisa cuando ve al jugador.

#include "WingShot/Creatures/Eagle/EagleAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"

AEagleAIController::AEagleAIController()
{
	// El aguila no se asusta.
	bCanBeScared = false;
	// Vista de 40 m.
	SightConfig->SightRadius = 4000.f;
	SightConfig->LoseSightRadius = 4500.f;
	SightConfig->PeripheralVisionAngleDegrees = 85.f;
	AIPerception->ConfigureSense(*SightConfig);
}

void AEagleAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	Super::HandleTargetPerceptionUpdated(Actor, Stimulus);

	const APawn* PerceivedPawn = Cast<APawn>(Actor);
	if (!PerceivedPawn || !PerceivedPawn->IsPlayerControlled())
	{
		return;
	}

	// Al ver al jugador, avisa al StateTree: si esta cazando puede cambiar su curso para atacar.
	const float Now = GetWorld()->GetTimeSeconds();
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>() && Stimulus.WasSuccessfullySensed() && Now - LastSpotTime > SpotCooldown)
	{
		LastSpotTime = Now;
		StateTreeComponent->SendStateTreeEvent(GetSpottedPlayerEventTag());
		UE_LOG(LogTemp, Display, TEXT("%s: vio al jugador"), *GetNameSafe(GetPawn()));
	}
}
