#include "WingShot/Creatures/Mouse/MouseCritter.h"
#include "WingShot/Creatures/Mouse/MouseAIController.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTree.h"

AMouseCritter::AMouseCritter()
{
	// Se controla con MouseAIController.
	AIControllerClass = AMouseAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	BehaviorTreeAsset = TSoftObjectPtr<UStateTree>(FSoftObjectPath(TEXT("/Game/WingShot/AI/Mouse/ST_Mouse.ST_Mouse")));

	GetCapsuleComponent()->InitCapsuleSize(14.f, 14.f);
	GetCharacterMovement()->MaxWalkSpeed = 260.f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
	bUseControllerRotationYaw = false;
}

UStateTree* AMouseCritter::GetBehaviorStateTree() const
{
	return BehaviorTreeAsset.LoadSynchronous();
}

void AMouseCritter::SetCurrentState(FName NewState)
{
	CurrentState = NewState;
	// Avisa al Blueprint para que actualice su texto.
	OnStateChanged(NewState);
}

// Cazado por un buho o aguila.
void AMouseCritter::Kill(AActor* Killer)
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Killed"));
		}
	}

	// Muerte: sin movimiento ni colision, y desaparece despues de unos segundos.
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	// Avisa al Blueprint (animacion o efecto de muerte).
	OnKilled();
	SetLifeSpan(4.f);
	UE_LOG(LogTemp, Display, TEXT("%s fue cazado por %s"), *GetName(), *GetNameSafe(Killer));
}
