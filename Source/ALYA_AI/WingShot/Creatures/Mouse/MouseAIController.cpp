#include "WingShot/Creatures/Mouse/MouseAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "StateTree.h"
#include "WingShot/Creatures/Eagle/EagleBird.h"
#include "WingShot/Creatures/Mouse/MouseCritter.h"
#include "WingShot/Creatures/Owl/OwlBird.h"
#include "WingShot/Creatures/Shared/CreatureCommon.h"

AMouseAIController::AMouseAIController()
{
	// el StateTree se arranca en OnPossess
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	// Vista y oido a ras de suelo, detecta al jugador y a los depredadores cercanos
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 900.f;
	SightConfig->LoseSightRadius = 1100.f;
	SightConfig->PeripheralVisionAngleDegrees = 100.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 1000.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
	SetPerceptionComponent(*AIPerception);
}

void AMouseAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Amenazas, el jugador y los depredadores (buho y aguila)
	const APawn* PerceivedPawn = Cast<APawn>(Actor);
	const bool bThreat = PerceivedPawn && (PerceivedPawn->IsPlayerControlled() || Actor->IsA<AOwlBird>() || Actor->IsA<AEagleBird>());
	if (!bThreat || !Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	// aturdido no reacciona, el stun no se interrumpe con un susto
	if (const AMouseCritter* Mouse = Cast<AMouseCritter>(GetPawn()); Mouse && Mouse->IsStunned())
	{
		return;
	}

	ThreatLocation = Stimulus.StimulusLocation;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastScareTime > ScareCooldown)
	{
		LastScareTime = Now;
		// evento -> el StateTree pasa a Flee
		StateTreeComponent->SendStateTreeEvent(WingShot::ScaredTag());
	}
}

void AMouseAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// Escucha la percepcion y arranca ST_Mouse
	AIPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AMouseAIController::HandleTargetPerceptionUpdated);
	if (const AMouseCritter* Mouse = Cast<AMouseCritter>(InPawn))
	{
		if (UStateTree* Tree = Mouse->GetBehaviorStateTree())
		{
			StateTreeComponent->SetStateTree(Tree);
			StateTreeComponent->StartLogic();
		}
	}
}
