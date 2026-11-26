// AIController base de las aves. Lo heredan las tres especies.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionTypes.h"
#include "WingShot/Creatures/Shared/BirdTypes.h"
#include "BirdAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UStateTreeAIComponent;

// Controlador de las aves: percepcion (vista + oido), sustos y memoria de Markov.
UCLASS()
class ALYA_AI_API ABirdAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABirdAIController();

	// Revisa si el jugador (visto por AIPerception) esta demasiado cerca. La llama el Evaluator
	// "Bird Senses" en intervalos escalonados: no hay logica sensorial en el Tick del controller.
	void CheckPlayerProximity();

	// Tag del evento que se manda al StateTree cuando el ave se asusta.
	static FGameplayTag GetScaredEventTag();

	// Tag del evento que se manda cuando un depredador ve al jugador (aguila).
	static FGameplayTag GetSpottedPlayerEventTag();

	// --- Datos que leen las tareas, evaluators y contextos EQS ---
	const FVector& GetThreatLocation() const { return ThreatLocation; }
	AActor* GetThreatActor() const { return ThreatActor.Get(); }
	bool IsPlayerVisible() const { return bPlayerVisible; }

	// Recientemente asustada: la cadena de Markov usa la fila "Nervous".
	bool IsNervous() const;

	EBirdActivity GetLastActivity() const { return LastActivity; }
	void SetLastActivity(EBirdActivity Activity) { LastActivity = Activity; }

protected:
	// Al tomar control del ave arranca su StateTree.
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	// Se llama cuando AI Perception ve u oye algo.
	virtual void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// Asusta al ave: manda el evento Scared al StateTree.
	void Scare(const FVector& FromLocation, AActor* FromActor);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	// Ejecuta el StateTree de la especie.
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	// Percepcion: vista + oido.
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	// Si es falso el ave nunca se asusta (aguila).
	UPROPERTY(EditAnywhere, Category = "AI|Fear")
	bool bCanBeScared = true;

	// Estando dormida solo despierta con ruidos de al menos esta intensidad (los pasos en sigilo son 0.2).
	UPROPERTY(EditAnywhere, Category = "AI|Fear")
	float WakeUpLoudness = 0.5f;

	// Distancia a la que ver al jugador asusta al ave (caminando / en sigilo).
	UPROPERTY(EditAnywhere, Category = "AI|Fear")
	float SightScareDistance = 1000.f;

	UPROPERTY(EditAnywhere, Category = "AI|Fear")
	// Lo mismo pero si el jugador va agachado.
	float SneakSightScareDistance = 450.f;

	// Segundos que el ave sigue "nerviosa" despues de un susto.
	UPROPERTY(EditAnywhere, Category = "AI|Fear")
	float NervousDuration = 20.f;

	// Tiempo minimo entre dos eventos de susto.
	UPROPERTY(EditAnywhere, Category = "AI|Fear")
	float ScareCooldown = 1.5f;

	// Ultima amenaza percibida.
	TWeakObjectPtr<AActor> ThreatActor;
	FVector ThreatLocation = FVector::ZeroVector;
	bool bPlayerVisible = false;
	float LastScareTime = -1000.f;

private:
	// Ultima actividad elegida por Markov.
	EBirdActivity LastActivity = EBirdActivity::Perch;
};
