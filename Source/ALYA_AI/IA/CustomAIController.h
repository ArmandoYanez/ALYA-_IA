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

	bool CanSeePlayer() const { return bCanSeePlayer; }
	AActor* GetPerceivedPlayer() const { return PerceivedPlayer; }
	const FVector& GetLastKnownPlayerLocation() const { return LastKnownPlayerLocation; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void ClearPlayerSight();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LoseSightGraceTime = 1.0f;

	FTimerHandle LoseSightTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	bool bCanSeePlayer = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<AActor> PerceivedPlayer = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	FVector LastKnownPlayerLocation = FVector::ZeroVector;
};
