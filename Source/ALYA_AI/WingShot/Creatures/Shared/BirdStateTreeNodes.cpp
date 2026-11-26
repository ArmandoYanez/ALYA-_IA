#include "WingShot/Creatures/Shared/BirdStateTreeNodes.h"

#include "AIController.h"
#include "Engine/DataTable.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "WingShot/Creatures/Shared/BirdAIController.h"
#include "WingShot/Creatures/Shared/BirdBase.h"
#include "WingShot/World/BirdPerch.h"
#include "WingShot/Creatures/Mouse/MouseCritter.h"
#include "WingShot/Creatures/Mouse/MouseAIController.h"
#include "WingShot/Creatures/Owl/OwlBird.h"
#include "WingShot/Creatures/Eagle/EagleBird.h"
#include "WingShot/Creatures/Eagle/EagleAIController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "WingShot/Creatures/Shared/CreatureCommon.h"

// Markov
EStateTreeRunStatus FStateTreeTask_BirdMarkovDecide::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ABirdAIController* Controller = Cast<ABirdAIController>(Data.AIController);
	const ABirdBase* Bird = Controller ? Cast<ABirdBase>(Controller->GetPawn()) : nullptr;
	if (!Controller || !Bird || !Data.MarkovTable)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 1. Fila de la matriz, la actividad actual, o "Nervous" si se asusto hace poco
	const UEnum* ActivityEnum = StaticEnum<EBirdActivity>();
	FName CurrentRow = FName(*ActivityEnum->GetNameStringByValue(static_cast<int64>(Controller->GetLastActivity())));
	if (!Data.RowOverride.IsNone())
	{
		CurrentRow = Data.RowOverride;
	}
	else if (Controller->IsNervous() && Data.MarkovTable->FindRow<FBirdMarkovRow>(TEXT("Nervous"), TEXT("BirdMarkov"), false))
	{
		CurrentRow = TEXT("Nervous");
	}

	const FBirdMarkovRow* Row = Data.MarkovTable->FindRow<FBirdMarkovRow>(CurrentRow, TEXT("BirdMarkov"), false);
	if (!Row)
	{
		// Fila inicial, la primera de la tabla
		const TArray<FName> RowNames = Data.MarkovTable->GetRowNames();
		Row = RowNames.Num() > 0 ? Data.MarkovTable->FindRow<FBirdMarkovRow>(RowNames[0], TEXT("BirdMarkov")) : nullptr;
	}
	if (!Row)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 2. Normalizacion, la fila debe sumar 1.0; si no, se avisa y se escala el muestreo
	float RowSum = 0.f;
	for (int32 Index = 0; Index < ActivityEnum->NumEnums() - 1; ++Index)
	{
		RowSum += Row->Get(static_cast<EBirdActivity>(ActivityEnum->GetValueByIndex(Index)));
	}
	if (RowSum <= KINDA_SMALL_NUMBER)
	{
		return EStateTreeRunStatus::Failed;
	}
	if (!FMath::IsNearlyEqual(RowSum, 1.f, 0.01f))
	{
		UE_LOG(LogTemp, Warning, TEXT("Markov: la fila %s suma %.3f (deberia ser 1.0); se normaliza."), *CurrentRow.ToString(), RowSum);
	}

	// 3. Muestreo, numero aleatorio contra las probabilidades acumuladas
	const float RandomValue = FMath::FRand() * RowSum;
	float Accumulated = 0.f;
	EBirdActivity Next = EBirdActivity::Perch;
	for (int32 Index = 0; Index < ActivityEnum->NumEnums() - 1; ++Index)
	{
		const EBirdActivity Candidate = static_cast<EBirdActivity>(ActivityEnum->GetValueByIndex(Index));
		Accumulated += Row->Get(Candidate);
		if (RandomValue < Accumulated)
		{
			Next = Candidate;
			break;
		}
	}

	// Desde el suelo no se puede "seguir posada", primero tiene que volar a una rama
	if (Next == EBirdActivity::Perch && Bird->IsOnGround())
	{
		Next = EBirdActivity::Relocate;
	}

	// Cazar solo si hay un raton vivo; atacar solo si hay una amenaza conocida cerca
	if (Next == EBirdActivity::Hunt)
	{
		bool bPreyAlive = false;
		for (TActorIterator<AMouseCritter> It(Bird->GetWorld()); It; ++It)
		{
			bPreyAlive |= It->IsAlive();
		}
		if (!bPreyAlive)
		{
			Next = Data.FallbackActivity;
		}
	}
	if (Next == EBirdActivity::Attack)
	{
		const AActor* Threat = Controller->GetThreatActor();
		if (!Threat || FVector::Dist(Threat->GetActorLocation(), Bird->GetActorLocation()) > 3500.f)
		{
			Next = Data.FallbackActivity;
		}
	}

	// Guarda el resultado; las transiciones del StateTree lo leen
	Data.NextActivity = Next;
	// Recuerda la actividad para la siguiente decision
	Controller->SetLastActivity(Next);

	UE_LOG(LogTemp, Display, TEXT("%s Markov [%s] R=%.2f -> %s"), *Bird->GetName(), *CurrentRow.ToString(), RandomValue,
		*ActivityEnum->GetNameStringByValue(static_cast<int64>(Next)));
	return EStateTreeRunStatus::Succeeded;
}

// Find Location (EQS)
EStateTreeRunStatus FStateTreeTask_BirdFindLocation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bFound = false;
	Data.TargetActor = nullptr;
	Data.Status = EStateTreeRunStatus::Running;

	if (!Data.Pawn || !Data.Query)
	{
		return EStateTreeRunStatus::Failed;
	}

	// lanza el EQS de forma asincrona (no congela el juego)
	FEnvQueryRequest Request(Data.Query, Data.Pawn);
	Data.RequestId = Request.Execute(EEnvQueryRunMode::SingleResult,
		FQueryFinishedSignature::CreateLambda([WeakContext = Context.MakeWeakExecutionContext()](TSharedPtr<FEnvQueryResult> Result) mutable
		{
			const FStateTreeStrongExecutionContext Strong = WeakContext.MakeStrongExecutionContext();
			FInstanceDataType* Data = Strong.GetInstanceDataPtr<FInstanceDataType>();
			if (!Data)
			{
				return;
			}
			Data->RequestId = INDEX_NONE;

			if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.Num() == 0)
			{
				Data->Status = EStateTreeRunStatus::Failed;
				Strong.FinishTask(EStateTreeFinishTaskType::Failed);
				return;
			}

			// Toma el mejor resultado
			AActor* Actor = Result->GetItemAsActor(0);
			Data->TargetActor = Actor;
			Data->Location = Result->GetItemAsLocation(0);

			if (ABirdBase* Bird = Cast<ABirdBase>(Data->Pawn))
			{
				if (ABirdPerch* Perch = Cast<ABirdPerch>(Actor))
				{
					// Aparta la rama para que otra ave no la use
					Bird->ReservePerch(Perch);
				}
			}
			if (const ABirdFeedSpot* Spot = Cast<ABirdFeedSpot>(Actor))
			{
				// En una zona de comida elige un punto al azar
				Data->Location = Spot->GetRandomFeedLocation();
			}

			// No termina, los estados hijos leen Location/TargetActor por binding
			Data->bFound = true;
		}));

	return Data.RequestId == INDEX_NONE && !Data.bFound ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_BirdFindLocation::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// sigue corriendo mientras el EQS no falle
	return Context.GetInstanceData(*this).Status;
}

void FStateTreeTask_BirdFindLocation::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.RequestId != INDEX_NONE)
	{
		if (UEnvQueryManager* Manager = UEnvQueryManager::GetCurrent(Data.Pawn))
		{
			// Si salimos del estado antes, cancela el EQS
			Manager->AbortQuery(Data.RequestId);
		}
		Data.RequestId = INDEX_NONE;
	}
}

// Fly To
bool FStateTreeTask_BirdFlyTo::TryStart(FInstanceDataType& Data) const
{
	ABirdBase* Bird = Cast<ABirdBase>(Data.Pawn);
	if (!Bird || !Data.bReady)
	{
		return false;
	}
	// Empieza el vuelo en arco
	Bird->FlyTo(Data.Destination, Data.Speed, Data.ArcHeight);
	Data.bStarted = true;
	return true;
}

EStateTreeRunStatus FStateTreeTask_BirdFlyTo::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bStarted = false;
	if (!Cast<ABirdBase>(Data.Pawn))
	{
		return EStateTreeRunStatus::Failed;
	}
	TryStart(Data);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_BirdFlyTo::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ABirdBase* Bird = Cast<ABirdBase>(Data.Pawn);
	if (!Bird)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Espera a que el EQS del estado padre tenga resultado
	if (!Data.bStarted)
	{
		TryStart(Data);
		return EStateTreeRunStatus::Running;
	}

	// Mientras vuela, si persigue una presa actualiza el destino
	if (Bird->IsFlying())
	{
		if (Data.bTrackTarget && IsValid(Data.TargetActor))
		{
			Bird->UpdateFlightTarget(Data.TargetActor->GetActorLocation());
		}
		return EStateTreeRunStatus::Running;
	}

	// llego, se posa (o atrapa al raton)
	Bird->OnArrived(Data.TargetActor);
	return EStateTreeRunStatus::Succeeded;
}

void FStateTreeTask_BirdFlyTo::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (ABirdBase* Bird = Cast<ABirdBase>(Data.Pawn))
	{
		if (Bird->IsFlying())
		{
			// Si el estado se interrumpe, deja de volar
			Bird->StopFlight();
		}
	}
}

// Idle
EStateTreeRunStatus FStateTreeTask_BirdIdle::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	// Tiempo de espera al azar
	Data.Remaining = FMath::FRandRange(Data.MinTime, FMath::Max(Data.MinTime, Data.MaxTime));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_BirdIdle::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	// Cuenta regresiva; en 0 termina
	Data.Remaining -= DeltaTime;
	return Data.Remaining <= 0.f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

// condition
bool FStateTreeCondition_BirdActivity::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	// Se cumple si Markov eligio esta actividad
	return Data.Activity == Data.Expected;
}

// Evaluator
void FStateTreeEvaluator_BirdSenses::TreeStart(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	// Castea el controller una sola vez
	Data.CachedController = Cast<ABirdAIController>(Data.AIController);
	// Desfase aleatorio, cada ave revisa en un frame distinto (intervalos escalonados)
	Data.TimeToNextCheck = FMath::FRandRange(0.f, Data.CheckInterval);
}

void FStateTreeEvaluator_BirdSenses::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ABirdAIController* Controller = Data.CachedController.Get();
	if (!Controller)
	{
		return;
	}

	// Solo revisa cada CheckInterval segundos, no cada frame
	Data.TimeToNextCheck -= DeltaTime;
	if (Data.TimeToNextCheck > 0.f)
	{
		return;
	}
	Data.TimeToNextCheck = Data.CheckInterval;

	// copia lo que percibe el controller
	Data.bPlayerVisible = Controller->IsPlayerVisible();
	Data.bNervous = Controller->IsNervous();
	Data.ThreatLocation = Controller->GetThreatLocation();
	// Si el jugador esta muy cerca, el ave se asusta
	Controller->CheckPlayerProximity();
}

// Attack Player
EStateTreeRunStatus FStateTreeTask_BirdAttackPlayer::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ABirdBase* Bird = Cast<ABirdBase>(Data.Pawn);
	APawn* Player = Bird ? UGameplayStatics::GetPlayerPawn(Bird, 0) : nullptr;
	if (!Bird || !Player)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.Target = Player;
	Data.Elapsed = 0.f;
	// Se lanza hacia el jugador
	Bird->FlyTo(Player->GetActorLocation(), Data.Speed, 120.f);
	if (AEagleAIController* Eagle = Cast<AEagleAIController>(Bird->GetController()))
	{
		// mientras ataca, el aguila no se puede atrapar
		Eagle->SetAttacking(true);
	}
	UE_LOG(LogTemp, Display, TEXT("%s ataca al jugador"), *Bird->GetName());
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_BirdAttackPlayer::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ABirdBase* Bird = Cast<ABirdBase>(Data.Pawn);
	AActor* Target = Data.Target.Get();
	if (!Bird || !Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed += DeltaTime;
	const FVector TargetLocation = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	if (FVector::Dist(Bird->GetActorLocation(), TargetLocation) < 150.f)
	{
		// Lo alcanzo, le hace dano
		UGameplayStatics::ApplyDamage(Target, Data.Damage, Bird->GetController(), Bird, nullptr);
		return EStateTreeRunStatus::Succeeded;
	}
	// si tarda mucho, se rinde
	if (Data.Elapsed > Data.MaxTime)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Sigue al jugador, si termino el vuelo sin alcanzarlo, vuelve a lanzarse
	if (Bird->IsFlying())
	{
		Bird->UpdateFlightTarget(TargetLocation);
	}
	else
	{
		Bird->FlyTo(TargetLocation, Data.Speed, 60.f);
	}
	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_BirdAttackPlayer::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (ABirdBase* Bird = Cast<ABirdBase>(Context.GetInstanceData(*this).Pawn))
	{
		Bird->StopFlight();
		if (AEagleAIController* Eagle = Cast<AEagleAIController>(Bird->GetController()))
		{
			Eagle->SetAttacking(false);
		}
	}
}

// random Chance
bool FStateTreeCondition_RandomChance::TestCondition(FStateTreeExecutionContext& Context) const
{
	// Numero al azar entre 0 y 1 contra la probabilidad
	return FMath::FRand() < Context.GetInstanceData(*this).Probability;
}

// State Label
void FStateTreeEvaluator_StateLabel::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.TimeToNextUpdate -= DeltaTime;
	if (Data.TimeToNextUpdate > 0.f)
	{
		return;
	}
	// Actualiza el texto 5 veces por segundo
	Data.TimeToNextUpdate = 0.2f;

	// Nombre del estado activo mas profundo
	const TArray<FName> ActiveStates = Context.GetActiveStateNames();
	const FName Leaf = ActiveStates.Num() > 0 ? ActiveStates.Last() : NAME_None;
	if (Leaf == Data.ActiveState)
	{
		return;
	}
	Data.ActiveState = Leaf;

	// avisa a la criatura; su Blueprint decide como mostrarlo (OnStateChanged)
	if (ABirdBase* Bird = Cast<ABirdBase>(Data.Pawn))
	{
		Bird->SetCurrentState(Leaf);
	}
	else if (AMouseCritter* Mouse = Cast<AMouseCritter>(Data.Pawn))
	{
		Mouse->SetCurrentState(Leaf);
	}
}

// Creature Stunned
namespace
{
	// activa o quita el stun en un ave o un raton
	void SetCreatureStunned(APawn* Pawn, bool bStunned)
	{
		if (ABirdBase* Bird = Cast<ABirdBase>(Pawn))
		{
			Bird->SetStunned(bStunned);
		}
		else if (AMouseCritter* Mouse = Cast<AMouseCritter>(Pawn))
		{
			Mouse->SetStunned(bStunned);
			if (bStunned)
			{
				if (AAIController* Controller = Cast<AAIController>(Mouse->GetController()))
				{
					Controller->StopMovement();
				}
			}
		}
	}
}

EStateTreeRunStatus FStateTreeTask_CreatureStunned::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Remaining = Data.Duration;
	// Al entrar, queda aturdido
	SetCreatureStunned(Data.Pawn, true);
	UE_LOG(LogTemp, Display, TEXT("%s quedo aturdido"), *GetNameSafe(Data.Pawn));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_CreatureStunned::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Remaining -= DeltaTime;
	return Data.Remaining <= 0.f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FStateTreeTask_CreatureStunned::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// al salir, se recupera
	SetCreatureStunned(Context.GetInstanceData(*this).Pawn, false);
}

// Precondiciones GOAP
// Precondicion de Cazar, existe un raton vivo
bool FStateTreeCondition_PreyAvailable::TestCondition(FStateTreeExecutionContext& Context) const
{
	const APawn* Pawn = Context.GetInstanceData(*this).Pawn;
	if (!Pawn)
	{
		return false;
	}
	for (TActorIterator<AMouseCritter> It(Pawn->GetWorld()); It; ++It)
	{
		if (It->IsAlive())
		{
			return true;
		}
	}
	return false;
}

// Precondicion de Atacar, la amenaza esta dentro del rango
bool FStateTreeCondition_ThreatInRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	const ABirdAIController* Controller = Cast<ABirdAIController>(Data.AIController);
	const APawn* Bird = Controller ? Controller->GetPawn() : nullptr;
	const AActor* Threat = Controller ? Controller->GetThreatActor() : nullptr;
	return Bird && Threat && FVector::Dist(Bird->GetActorLocation(), Threat->GetActorLocation()) <= Data.Range;
}
