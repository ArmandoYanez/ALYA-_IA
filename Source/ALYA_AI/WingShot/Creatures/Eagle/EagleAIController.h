#pragma once

#include "CoreMinimal.h"
#include "WingShot/Creatures/Shared/BirdAIController.h"
#include "EagleAIController.generated.h"

// Controlador del aguila: vista de largo alcance, nunca se asusta y avisa al StateTree cuando ve al jugador.
UCLASS()
class ALYA_AI_API AEagleAIController : public ABirdAIController
{
	GENERATED_BODY()

public:
	AEagleAIController();

	bool IsAttacking() const { return bAttacking; }
	void SetAttacking(bool bInAttacking) { bAttacking = bInAttacking; }

protected:
	virtual void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus) override;

	// Tiempo minimo entre dos avisos de "vio al jugador".
	UPROPERTY(EditAnywhere, Category = "AI|Eagle")
	float SpotCooldown = 6.f;

private:
	bool bAttacking = false;
	float LastSpotTime = -1000.f;
};
