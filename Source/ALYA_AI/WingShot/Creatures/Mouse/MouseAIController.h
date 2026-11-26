// Controller del raton, percepcion y su StateTree

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "MouseAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UStateTreeAIComponent;

// controlador del raton, ejecuta su StateTree y percibe al jugador y a los depredadores para huir
UCLASS()
class ALYA_AI_API AMouseAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMouseAIController();

	const FVector& GetThreatLocation() const { return ThreatLocation; }

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	// Tiempo minimo entre dos sustos
	UPROPERTY(EditAnywhere, Category = "AI|Fear")
	float ScareCooldown = 2.f;

private:
	FVector ThreatLocation = FVector::ZeroVector;
	float LastScareTime = -1000.f;
};
