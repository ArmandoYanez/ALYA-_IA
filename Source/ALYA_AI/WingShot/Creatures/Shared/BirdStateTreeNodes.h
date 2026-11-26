// Nodos de StateTree compartidos (tareas, condiciones y evaluadores).
// Se usan en ST_CommonBird, ST_Owl, ST_Eagle y ST_Mouse.
// Cada nodo tiene un struct "InstanceData" con sus variables y otro con la logica.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeTaskBase.h"
#include "WingShot/Creatures/Shared/BirdTypes.h"
#include "BirdStateTreeNodes.generated.h"

class AAIController;
class UDataTable;
class UEnvQuery;

// =====================================================================
// Cadena de Markov: elige la siguiente actividad del ave.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeTask_BirdMarkovDecideInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	// Controller del ave (lo llena el StateTree).
	TObjectPtr<AAIController> AIController = nullptr;

	// Matriz de transicion (filas FBirdMarkovRow). Nombre de fila = actividad actual.
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UDataTable> MarkovTable = nullptr;

	// Si se llena, usa siempre esta fila (por ejemplo "Disturbed" para atacar o huir al despertar).
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName RowOverride;

	// Actividad que se usa si la elegida no es posible (Cazar sin ratones, Atacar sin amenaza).
	UPROPERTY(EditAnywhere, Category = "Parameter")
	EBirdActivity FallbackActivity = EBirdActivity::Perch;

	UPROPERTY(EditAnywhere, Category = "Output")
	// Resultado: la actividad elegida.
	EBirdActivity NextActivity = EBirdActivity::Perch;
};

USTRUCT(meta = (DisplayName = "Bird Markov Decide", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeTask_BirdMarkovDecide : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_BirdMarkovDecideInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	// Se ejecuta una vez al entrar al estado: decide y termina.
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// =====================================================================
// Ejecuta un EQS y expone el mejor punto. Sigue corriendo para que los
// estados hijos lean su resultado por Property Binding.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeTask_BirdFindLocationInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	// EQS que se va a ejecutar.
	TObjectPtr<UEnvQuery> Query = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	// Punto encontrado por el EQS.
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	// True cuando el EQS ya respondio.
	bool bFound = false;

	// Id de la consulta EQS (para poder cancelarla).
	int32 RequestId = INDEX_NONE;
	EStateTreeRunStatus Status = EStateTreeRunStatus::Running;
};

USTRUCT(meta = (DisplayName = "Bird Find Location (EQS)", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeTask_BirdFindLocation : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_BirdFindLocationInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// =====================================================================
// Vuela hasta Destination en cuanto bReady sea verdadero.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeTask_BirdFlyToInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	// A donde volar (viene del EQS).
	FVector Destination = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	// True cuando el destino ya es valido.
	bool bReady = false;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Speed = 650.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	// Altura del arco de vuelo.
	float ArcHeight = 250.f;

	// Sigue al TargetActor mientras vuela (presas que caminan).
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bTrackTarget = false;

	// Ya empezo a volar.
	bool bStarted = false;
};

USTRUCT(meta = (DisplayName = "Bird Fly To", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeTask_BirdFlyTo : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_BirdFlyToInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	bool TryStart(FInstanceDataType& InstanceData) const;
};

// =====================================================================
// Espera un tiempo aleatorio (posada en la rama o comiendo).
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeTask_BirdIdleInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MinTime = 3.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MaxTime = 6.f;

	// Tiempo que falta.
	float Remaining = 0.f;
};

USTRUCT(meta = (DisplayName = "Bird Idle", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeTask_BirdIdle : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_BirdIdleInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// =====================================================================
// Ataque en picada al jugador: lo persigue volando y le hace dano al alcanzarlo.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeTask_BirdAttackPlayerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Speed = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	// Dano que le hace al jugador.
	float Damage = 20.f;

	// Si no alcanza al jugador en este tiempo, desiste.
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MaxTime = 6.f;

	float Elapsed = 0.f;
	TWeakObjectPtr<AActor> Target;
};

USTRUCT(meta = (DisplayName = "Bird Attack Player", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeTask_BirdAttackPlayer : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_BirdAttackPlayerInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// =====================================================================
// Condicion: se cumple con una probabilidad dada (decisiones al vuelo).
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeCondition_RandomChanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Probability = 0.5f;
};

USTRUCT(meta = (DisplayName = "Random Chance", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeCondition_RandomChance : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_RandomChanceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// =====================================================================
// Condicion: la actividad elegida por Markov es la esperada.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeCondition_BirdActivityInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	EBirdActivity Activity = EBirdActivity::Perch;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EBirdActivity Expected = EBirdActivity::Perch;
};

USTRUCT(meta = (DisplayName = "Bird Activity Is", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeCondition_BirdActivity : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_BirdActivityInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// =====================================================================
// Evaluator: expone al StateTree lo que percibe el ave.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeEvaluator_BirdSensesInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	// Cada cuanto se revisa la distancia al jugador (se desfasa al azar entre aves).
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.05"))
	float CheckInterval = 0.25f;

	// Controller ya casteado (se resuelve una sola vez en TreeStart).
	TWeakObjectPtr<class ABirdAIController> CachedController;
	float TimeToNextCheck = 0.f;

	UPROPERTY(EditAnywhere, Category = "Output")
	// Ve al jugador?
	bool bPlayerVisible = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	// Se asusto hace poco?
	bool bNervous = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	// Donde esta la amenaza.
	FVector ThreatLocation = FVector::ZeroVector;
};

USTRUCT(meta = (DisplayName = "Bird Senses", Category = "WingShot|Bird"))
struct ALYA_AI_API FStateTreeEvaluator_BirdSenses : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEvaluator_BirdSensesInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// =====================================================================
// Evaluator: le pasa a la criatura el estado activo del StateTree (su Blueprint lo muestra).
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeEvaluator_StateLabelInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	// Ultimo estado mostrado.
	FName ActiveState;

	float TimeToNextUpdate = 0.f;
};

USTRUCT(meta = (DisplayName = "State Label", Category = "WingShot|Creature"))
struct ALYA_AI_API FStateTreeEvaluator_StateLabel : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEvaluator_StateLabelInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// =====================================================================
// Reaccion a dano/stun: la criatura queda aturdida unos segundos.
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeTask_CreatureStunnedInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	// Segundos que dura aturdido.
	float Duration = 2.5f;

	float Remaining = 0.f;
};

USTRUCT(meta = (DisplayName = "Creature Stunned", Category = "WingShot|Creature"))
struct ALYA_AI_API FStateTreeTask_CreatureStunned : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_CreatureStunnedInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// =====================================================================
// Precondiciones estilo GOAP (Enter Conditions).
// =====================================================================
USTRUCT()
struct ALYA_AI_API FStateTreeCondition_PreyAvailableInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
};

// Cazar solo si existe al menos un raton vivo en el mapa.
USTRUCT(meta = (DisplayName = "Prey Available (GOAP)", Category = "WingShot|Preconditions"))
struct ALYA_AI_API FStateTreeCondition_PreyAvailable : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_PreyAvailableInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT()
struct ALYA_AI_API FStateTreeCondition_ThreatInRangeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	// Distancia maxima para atacar.
	float Range = 3500.f;
};

// Atacar solo si hay una amenaza conocida dentro del alcance.
USTRUCT(meta = (DisplayName = "Threat In Range (GOAP)", Category = "WingShot|Preconditions"))
struct ALYA_AI_API FStateTreeCondition_ThreatInRange : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_ThreatInRangeInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
