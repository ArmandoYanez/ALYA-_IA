#include "WingShot/Creatures/Shared/BirdAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "WingShot/Creatures/Shared/BirdBase.h"
#include "WingShot/Creatures/Shared/CreatureCommon.h"
#include "WingShot/Player/WingShotCharacter.h"

ABirdAIController::ABirdAIController()
{
	// El StateTree se arranca a mano en OnPossess (cada especie usa uno distinto).
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	// Vista: 15 m de alcance, cono de 75 grados.
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.f;
	SightConfig->LoseSightRadius = 1900.f;
	SightConfig->PeripheralVisionAngleDegrees = 75.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	// Oido: 20 m de alcance.
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 2000.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
	SetPerceptionComponent(*AIPerception);
}

FGameplayTag ABirdAIController::GetScaredEventTag()
{
	return WingShot::ScaredTag();
}

FGameplayTag ABirdAIController::GetSpottedPlayerEventTag()
{
	return WingShot::SpottedPlayerTag();
}

void ABirdAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Escucha los cambios de percepcion.
	AIPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ABirdAIController::HandleTargetPerceptionUpdated);

	// Cada especie trae su propio StateTree.
	if (const ABirdBase* Bird = Cast<ABirdBase>(InPawn))
	{
		if (UStateTree* Tree = Bird->GetBehaviorStateTree())
		{
			StateTreeComponent->SetStateTree(Tree);
			StateTreeComponent->StartLogic();
		}
	}
}

void ABirdAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	const APawn* PerceivedPawn = Cast<APawn>(Actor);
	// Solo le importa el jugador.
	if (!PerceivedPawn || !PerceivedPawn->IsPlayerControlled())
	{
		return;
	}

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		// La vista solo cambia cuando el jugador entra o sale del cono de vision.
		bPlayerVisible = Stimulus.WasSuccessfullySensed();
		ThreatActor = Actor;
		ThreatLocation = Stimulus.StimulusLocation;
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		// Dormida, solo un ruido fuerte la despierta (el sigilo no).
		const ABirdBase* Bird = Cast<ABirdBase>(GetPawn());
		if (Bird && Bird->IsSleeping() && Stimulus.Strength < WakeUpLoudness)
		{
			return;
		}
		// Escuchar pasos o un disparo asusta (el sigilo reduce el alcance del ruido).
		Scare(Stimulus.StimulusLocation, Actor);
	}
}

void ABirdAIController::CheckPlayerProximity()
{
	// Mientras el jugador este a la vista, se asusta si se acerca demasiado (dormida no ve nada).
	const ABirdBase* Bird = Cast<ABirdBase>(GetPawn());
	const AActor* Threat = ThreatActor.Get();
	if (!bPlayerVisible || !Bird || !Threat || Bird->IsSleeping())
	{
		return;
	}

	const AWingShotCharacter* Player = Cast<AWingShotCharacter>(Threat);
	const float ScareDistance = (Player && Player->IsSneaking()) ? SneakSightScareDistance : SightScareDistance;
	// Demasiado cerca: se asusta.
	if (FVector::Dist(Bird->GetActorLocation(), Threat->GetActorLocation()) < ScareDistance)
	{
		Scare(Threat->GetActorLocation(), ThreatActor.Get());
	}
}

void ABirdAIController::Scare(const FVector& FromLocation, AActor* FromActor)
{
	const float Now = GetWorld()->GetTimeSeconds();
	ThreatLocation = FromLocation;
	ThreatActor = FromActor;

	// Aturdida no reacciona: el stun no se interrumpe con un susto.
	const ABirdBase* Bird = Cast<ABirdBase>(GetPawn());
	if (!bCanBeScared || (Bird && Bird->IsStunned()) || Now - LastScareTime < ScareCooldown)
	{
		return;
	}
	LastScareTime = Now;

	// Evento -> el StateTree cambia al estado de escape.
	StateTreeComponent->SendStateTreeEvent(GetScaredEventTag());
	UE_LOG(LogTemp, Display, TEXT("%s: asustada por %s"), *GetNameSafe(GetPawn()), *GetNameSafe(FromActor));
}

bool ABirdAIController::IsNervous() const
{
	// Nerviosa si se asusto hace menos de NervousDuration segundos.
	return GetWorld()->GetTimeSeconds() - LastScareTime < NervousDuration;
}
