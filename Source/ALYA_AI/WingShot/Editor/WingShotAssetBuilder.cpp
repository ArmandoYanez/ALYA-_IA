// herramienta del editor que crea los assets de IA (DataTables de Markov, EQS y StateTrees)
// no se usa al jugar, solo sirve para generar/regenerar los assets

#include "WingShot/Editor/WingShotAssetBuilder.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Engine/DataTable.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Item.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ActorsOfClass.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Distance.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Dot.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Trace.h"
#include "FileHelpers.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorModule.h"
#include "StateTreeEditorSchema.h"
#include "StateTreeState.h"
#include "WingShot/Creatures/Shared/BirdAIController.h"
#include "WingShot/Creatures/Shared/BirdEQS.h"
#include "WingShot/World/BirdPerch.h"
#include "WingShot/Creatures/Shared/BirdStateTreeNodes.h"
#include "WingShot/Creatures/Mouse/MouseStateTreeNodes.h"
#include "WingShot/Creatures/Owl/OwlStateTreeNodes.h"
#include "WingShot/Creatures/Shared/BirdTypes.h"
#include "WingShot/Creatures/Mouse/MouseCritter.h"
#include "WingShot/Creatures/Mouse/MouseAIController.h"
#include "WingShot/Creatures/Shared/CreatureCommon.h"

namespace WingShotBuilder
{
	// Crea el asset o reutiliza el existente (para poder regenerar)
	template <typename T>
	T* CreateOrLoadAsset(const FString& PackagePath, const FString& Name, TArray<UPackage*>& OutPackages)
	{
		const FString LongName = PackagePath / Name;
		T* Asset = LoadObject<T>(nullptr, *(LongName + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Asset)
		{
			UPackage* Package = CreatePackage(*LongName);
			Asset = NewObject<T>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
			FAssetRegistryModule::AssetCreated(Asset);
		}
		Asset->Modify();
		Asset->MarkPackageDirty();
		OutPackages.AddUnique(Asset->GetPackage());
		return Asset;
	}

	// ---------- Markov ----------
	FBirdMarkovRow Row(std::initializer_list<TPair<EBirdActivity, float>> Values)
	{
		FBirdMarkovRow Result;
		for (const TPair<EBirdActivity, float>& Value : Values)
		{
			Result.Set(Value.Key, Value.Value);
		}
		return Result;
	}

	UDataTable* MakeMarkovTable(const FString& Dir, const FString& Name, std::initializer_list<TPair<FName, FBirdMarkovRow>> Rows, TArray<UPackage*>& Packages)
	{
		UDataTable* Table = CreateOrLoadAsset<UDataTable>(Dir, Name, Packages);
		Table->RowStruct = FBirdMarkovRow::StaticStruct();
		Table->EmptyTable();
		for (const TPair<FName, FBirdMarkovRow>& Entry : Rows)
		{
			Table->AddRow(Entry.Key, Entry.Value);
		}
		return Table;
	}

	// ---------- EQS ----------
	UEnvQueryOption* ResetQuery(UEnvQuery* Query, TSubclassOf<AActor> ActorClass, float Radius)
	{
		Query->GetOptionsMutable().Reset();
#if WITH_EDITORONLY_DATA
		Query->EdGraph = nullptr; // el editor de EQS reconstruye el grafico a partir de las opciones
#endif
		UEnvQueryOption* Option = NewObject<UEnvQueryOption>(Query);
		UEnvQueryGenerator_ActorsOfClass* Generator = NewObject<UEnvQueryGenerator_ActorsOfClass>(Option);
		Generator->SearchedActorClass = ActorClass;
		Generator->SearchRadius.DefaultValue = Radius;
		Generator->GenerateOnlyActorsInRadius.DefaultValue = true;
		Generator->SearchCenter = UEnvQueryContext_Querier::StaticClass();
		Option->Generator = Generator;
		Query->GetOptionsMutable().Add(Option);
		return Option;
	}

	void AddDistance(UEnvQueryOption* Option, TSubclassOf<UEnvQueryContext> To, EEnvTestPurpose::Type Purpose, float FilterMin, float ScoringFactor)
	{
		UEnvQueryTest_Distance* Test = NewObject<UEnvQueryTest_Distance>(Option);
		Test->DistanceTo = To;
		Test->TestPurpose = Purpose;
		Test->FilterType = EEnvTestFilterType::Minimum;
		Test->FloatValueMin.DefaultValue = FilterMin;
		Test->ScoringFactor.DefaultValue = ScoringFactor;
		Option->Tests.Add(Test);
	}

	template <typename TTest>
	void AddBoolTest(UEnvQueryOption* Option, EEnvTestPurpose::Type Purpose)
	{
		TTest* Test = NewObject<TTest>(Option);
		Test->TestPurpose = Purpose;
		Test->BoolValue.DefaultValue = true;
		Option->Tests.Add(Test);
	}

	void AddHiddenFromPlayer(UEnvQueryOption* Option)
	{
		// bool Match = true ("require hit"), prefiere puntos donde la geometria tapa la vista del jugador
		UEnvQueryTest_Trace* Test = NewObject<UEnvQueryTest_Trace>(Option);
		Test->Context = UEnvQueryContext_WingShotPlayer::StaticClass();
		Test->TestPurpose = EEnvTestPurpose::Score;
		Test->BoolValue.DefaultValue = true;
		Test->ItemHeightOffset.DefaultValue = 20.f;
		Test->ContextHeightOffset.DefaultValue = 60.f;
		Option->Tests.Add(Test);
	}

	void AddAheadOfQuerier(UEnvQueryOption* Option)
	{
		// dot entre la orientacion del ave y la direccion hacia el punto, prefiere lo que tiene enfrente
		UEnvQueryTest_Dot* Test = NewObject<UEnvQueryTest_Dot>(Option);
		// LineA, LineB y TestMode son protected, se asignan por reflexion (como lo hace el panel de detalles)
		FEnvDirection LineA;
		LineA.DirMode = EEnvDirection::Rotation;
		LineA.Rotation = UEnvQueryContext_Querier::StaticClass();
		FEnvDirection LineB;
		LineB.DirMode = EEnvDirection::TwoPoints;
		LineB.LineFrom = UEnvQueryContext_Querier::StaticClass();
		LineB.LineTo = UEnvQueryContext_Item::StaticClass();
		for (const TPair<FName, FEnvDirection*>& Line : { TPair<FName, FEnvDirection*>(TEXT("LineA"), &LineA), TPair<FName, FEnvDirection*>(TEXT("LineB"), &LineB) })
		{
			if (const FStructProperty* Property = FindFProperty<FStructProperty>(UEnvQueryTest_Dot::StaticClass(), Line.Key))
			{
				*Property->ContainerPtrToValuePtr<FEnvDirection>(Test) = *Line.Value;
			}
		}
		if (const FEnumProperty* ModeProperty = FindFProperty<FEnumProperty>(UEnvQueryTest_Dot::StaticClass(), TEXT("TestMode")))
		{
			ModeProperty->GetUnderlyingProperty()->SetIntPropertyValue(ModeProperty->ContainerPtrToValuePtr<void>(Test),
				static_cast<int64>(EEnvTestDot::Dot2D));
		}
		Test->TestPurpose = EEnvTestPurpose::Score;
		Test->ScoringFactor.DefaultValue = 1.f;
		Option->Tests.Add(Test);
	}

	// ---------- StateTree ----------
	bool Bind(UStateTreeEditorData* EditorData, const FGuid& SourceId, const TCHAR* SourceProp, const FGuid& TargetId, const TCHAR* TargetProp)
	{
		FPropertyBindingPath Source(SourceId);
		FPropertyBindingPath Target(TargetId);
		if (Source.FromString(SourceProp) && Target.FromString(TargetProp))
		{
			EditorData->AddPropertyBinding(Source, Target);
			return true;
		}
		return false;
	}

	UStateTreeEditorData* ResetStateTree(UStateTree* StateTree)
	{
		const TSubclassOf<UStateTreeSchema> SchemaClass = UStateTreeAIComponentSchema::StaticClass();
		FStateTreeEditorModule& Module = FStateTreeEditorModule::GetModule();
		UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTree, Module.GetEditorDataClass(SchemaClass.Get()), NAME_None, RF_Transactional);
		StateTree->EditorData = EditorData;
		EditorData->Schema = NewObject<UStateTreeSchema>(EditorData, SchemaClass, NAME_None, RF_Transactional);
		EditorData->EditorSchema = NewObject<UStateTreeEditorSchema>(EditorData, UStateTreeEditorSchema::StaticClass(), NAME_None, RF_Transactional);
		return EditorData;
	}

	FGuid AddDecide(UStateTreeState& State, UDataTable* Table, EBirdActivity Fallback, FName RowOverride = NAME_None)
	{
		auto& Node = State.AddTask<FStateTreeTask_BirdMarkovDecide>();
		Node.GetInstanceData().MarkovTable = Table;
		Node.GetInstanceData().FallbackActivity = Fallback;
		Node.GetInstanceData().RowOverride = RowOverride;
		return Node.ID;
	}

	FGuid AddFindLocation(UStateTreeState& State, UEnvQuery* Query)
	{
		auto& Node = State.AddTask<FStateTreeTask_BirdFindLocation>();
		Node.GetInstanceData().Query = Query;
		return Node.ID;
	}

	FGuid AddFlyTo(UStateTreeState& State, float Speed, float ArcHeight, bool bTrackTarget = false)
	{
		auto& Node = State.AddTask<FStateTreeTask_BirdFlyTo>();
		Node.GetInstanceData().Speed = Speed;
		Node.GetInstanceData().ArcHeight = ArcHeight;
		Node.GetInstanceData().bTrackTarget = bTrackTarget;
		return Node.ID;
	}

	// El vuelo del hijo toma su destino del EQS del padre (Property Binding)
	void BindFlightToSearch(UStateTreeEditorData* EditorData, const FGuid& FindId, const FGuid& FlyId)
	{
		Bind(EditorData, FindId, TEXT("Location"), FlyId, TEXT("Destination"));
		Bind(EditorData, FindId, TEXT("TargetActor"), FlyId, TEXT("TargetActor"));
		Bind(EditorData, FindId, TEXT("bFound"), FlyId, TEXT("bReady"));
	}

	// Estado padre con EQS + estado hijo que vuela al resultado
	UStateTreeState& AddSearchAndFly(UStateTreeEditorData* EditorData, UStateTreeState& Parent, FName ChildName, UEnvQuery* Query,
		float Speed, float ArcHeight, bool bTrackTarget = false)
	{
		const FGuid FindId = AddFindLocation(Parent, Query);
		UStateTreeState& Child = Parent.AddChildState(ChildName);
		BindFlightToSearch(EditorData, FindId, AddFlyTo(Child, Speed, ArcHeight, bTrackTarget));
		return Child;
	}

	void AddIdle(UStateTreeState& State, float MinTime, float MaxTime)
	{
		auto& Node = State.AddTask<FStateTreeTask_BirdIdle>();
		Node.GetInstanceData().MinTime = MinTime;
		Node.GetInstanceData().MaxTime = MaxTime;
	}

	void AddActivityTransition(UStateTreeEditorData* EditorData, UStateTreeState& From, const FGuid& DecideId, EBirdActivity Activity, const UStateTreeState& To)
	{
		FStateTreeTransition& Transition = From.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &To);
		auto& Condition = Transition.AddCondition<FStateTreeCondition_BirdActivity>();
		Condition.GetInstanceData().Expected = Activity;
		Bind(EditorData, DecideId, TEXT("NextActivity"), Condition.ID, TEXT("Activity"));
	}

	void GoTo(UStateTreeState& From, EStateTreeTransitionTrigger Trigger, const UStateTreeState& To)
	{
		From.AddTransition(Trigger, EStateTreeTransitionType::GotoState, &To);
	}

	FString Finish(UStateTree* StateTree, TArray<UPackage*>& Packages, const FString& Summary)
	{
		FStateTreeCompilerLog Log;
		const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(StateTree, Log);
		Log.DumpToLog(LogTemp);
		UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
		return FString::Printf(TEXT("%s -> %s %s"), *Summary, *StateTree->GetName(),
			bCompiled ? TEXT("compilado OK") : TEXT("CON ERRORES (ver Output Log)"));
	}

	// reaccion a dano/stun (ciclo base compartido), evento de impacto -> aturdido -> regresa a ReturnTo
	void AddStunReaction(UStateTreeEditorData* EditorData, UStateTreeState& Root, const UStateTreeState& ReturnTo)
	{
		UStateTreeState& Stunned = Root.AddChildState(TEXT("Stunned"));
		Stunned.AddTask<FStateTreeTask_CreatureStunned>();
		GoTo(Stunned, EStateTreeTransitionTrigger::OnStateCompleted, ReturnTo);
		Root.AddTransition(EStateTreeTransitionTrigger::OnEvent, WingShot::StunnedTag(),
			EStateTreeTransitionType::GotoState, &Stunned).Priority = EStateTreeTransitionPriority::Critical;
	}

	void AddCommonEvaluators(UStateTreeEditorData* EditorData, bool bBirdSenses)
	{
		if (bBirdSenses)
		{
			EditorData->AddEvaluator<FStateTreeEvaluator_BirdSenses>();
		}
		EditorData->AddEvaluator<FStateTreeEvaluator_StateLabel>();
	}

	UEnvQuery* GetEscapeQuery(TArray<UPackage*>& Packages)
	{
		// Escape, rama libre, muy lejos de la amenaza y escondida del jugador (compartido por aves)
		UEnvQuery* Query = CreateOrLoadAsset<UEnvQuery>(TEXT("/Game/WingShot/AI/CommonBird"), TEXT("EQS_Bird_FindEscapePerch"), Packages);
		UEnvQueryOption* Option = ResetQuery(Query, ABirdPerch::StaticClass(), 5000.f);
		AddBoolTest<UEnvQueryTest_PerchAvailable>(Option, EEnvTestPurpose::Filter);
		AddDistance(Option, UEnvQueryContext_BirdThreat::StaticClass(), EEnvTestPurpose::FilterAndScore, 1500.f, 1.f);
		AddHiddenFromPlayer(Option);
		return Query;
	}
}
#endif // WITH_EDITOR

FString UWingShotAssetBuilder::BuildAllAssets()
{
	return BuildMouseAssets() + TEXT("\n") + BuildCommonBirdAssets() + TEXT("\n") + BuildOwlAssets() + TEXT("\n") + BuildEagleAssets();
}

// Ave comun, tabla Markov, 3 EQS y ST_CommonBird
FString UWingShotAssetBuilder::BuildCommonBirdAssets()
{
#if WITH_EDITOR
	using namespace WingShotBuilder;
	using A = EBirdActivity;
	TArray<UPackage*> Packages;
	const FString Dir = TEXT("/Game/WingShot/AI/CommonBird");

	// ---------- Markov ----------
	UDataTable* Markov = MakeMarkovTable(Dir, TEXT("DT_Markov_CommonBird"), {
		{ TEXT("Perch"),    Row({ { A::Perch, 0.35f }, { A::Relocate, 0.40f }, { A::Feed, 0.25f } }) },
		{ TEXT("Relocate"), Row({ { A::Perch, 0.55f }, { A::Relocate, 0.20f }, { A::Feed, 0.25f } }) },
		{ TEXT("Feed"),     Row({ { A::Relocate, 0.75f }, { A::Feed, 0.25f } }) },
		{ TEXT("Nervous"),  Row({ { A::Perch, 0.25f }, { A::Relocate, 0.70f }, { A::Feed, 0.05f } }) },
	}, Packages);

	// ---------- EQS ----------
	UEnvQuery* FindPerch = CreateOrLoadAsset<UEnvQuery>(Dir, TEXT("EQS_Bird_FindPerch"), Packages);
	{
		// Rama nueva, libre, en otro arbol (>= 400) y lejos del jugador
		UEnvQueryOption* Option = ResetQuery(FindPerch, ABirdPerch::StaticClass(), 2500.f);
		AddBoolTest<UEnvQueryTest_PerchAvailable>(Option, EEnvTestPurpose::Filter);
		AddDistance(Option, UEnvQueryContext_Querier::StaticClass(), EEnvTestPurpose::FilterAndScore, 400.f, -1.f);
		AddDistance(Option, UEnvQueryContext_WingShotPlayer::StaticClass(), EEnvTestPurpose::Score, 0.f, 1.f);
	}
	UEnvQuery* FindEscape = GetEscapeQuery(Packages);
	UEnvQuery* FindFeed = CreateOrLoadAsset<UEnvQuery>(Dir, TEXT("EQS_Bird_FindFeedSpot"), Packages);
	{
		// Comida, zona en el suelo lejos del jugador y cerca del ave
		UEnvQueryOption* Option = ResetQuery(FindFeed, ABirdFeedSpot::StaticClass(), 3500.f);
		AddDistance(Option, UEnvQueryContext_WingShotPlayer::StaticClass(), EEnvTestPurpose::Filter, 900.f, 1.f);
		AddDistance(Option, UEnvQueryContext_Querier::StaticClass(), EEnvTestPurpose::Score, 0.f, -1.f);
	}

	// ---------- StateTree ----------
	UStateTree* StateTree = CreateOrLoadAsset<UStateTree>(Dir, TEXT("ST_CommonBird"), Packages);
	UStateTreeEditorData* EditorData = ResetStateTree(StateTree);
	AddCommonEvaluators(EditorData, true);

	UStateTreeState& Root = EditorData->AddRootState();
	UStateTreeState& Calm = Root.AddChildState(TEXT("Calm"));
	UStateTreeState& Scared = Root.AddChildState(TEXT("Scared"));

	UStateTreeState& Decide = Calm.AddChildState(TEXT("Decide"));
	UStateTreeState& Perch = Calm.AddChildState(TEXT("Perch"));
	UStateTreeState& Relocate = Calm.AddChildState(TEXT("Relocate"));
	UStateTreeState& Feed = Calm.AddChildState(TEXT("Feed"));

	const FGuid DecideId = AddDecide(Decide, Markov, A::Perch);
	AddActivityTransition(EditorData, Decide, DecideId, A::Perch, Perch);
	AddActivityTransition(EditorData, Decide, DecideId, A::Relocate, Relocate);
	AddActivityTransition(EditorData, Decide, DecideId, A::Feed, Feed);

	AddIdle(Perch, 3.f, 7.f);
	GoTo(Perch, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	UStateTreeState& FlyToBranch = AddSearchAndFly(EditorData, Relocate, TEXT("FlyToBranch"), FindPerch, 650.f, 250.f);
	GoTo(Relocate, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	GoTo(FlyToBranch, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	UStateTreeState& FlyDown = AddSearchAndFly(EditorData, Feed, TEXT("FlyDown"), FindFeed, 550.f, 150.f);
	UStateTreeState& Eat = Feed.AddChildState(TEXT("Eat"));
	GoTo(Feed, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	GoTo(FlyDown, EStateTreeTransitionTrigger::OnStateSucceeded, Eat);
	GoTo(FlyDown, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	AddIdle(Eat, 4.f, 8.f);
	GoTo(Eat, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	// Susto (evento de percepcion) -> rama de escape
	Calm.AddTransition(EStateTreeTransitionTrigger::OnEvent, ABirdAIController::GetScaredEventTag(),
		EStateTreeTransitionType::GotoState, &Scared).Priority = EStateTreeTransitionPriority::High;
	UStateTreeState& Escape = AddSearchAndFly(EditorData, Scared, TEXT("Escape"), FindEscape, 1300.f, 350.f);
	GoTo(Scared, EStateTreeTransitionTrigger::OnStateFailed, Calm);
	GoTo(Escape, EStateTreeTransitionTrigger::OnStateCompleted, Calm);

	AddStunReaction(EditorData, Root, Calm);

	return Finish(StateTree, Packages, TEXT("Ave comun: DT_Markov_CommonBird, 3 EQS"));
#else
	return TEXT("Solo disponible en el editor");
#endif
}

// buho, tabla Markov, EQS de perchas y presas, ST_Owl
FString UWingShotAssetBuilder::BuildOwlAssets()
{
#if WITH_EDITOR
	using namespace WingShotBuilder;
	using A = EBirdActivity;
	TArray<UPackage*> Packages;
	const FString Dir = TEXT("/Game/WingShot/AI/Owl");

	// ---------- Markov ----------
	// "Disturbed" es la decision al despertarlo, atacar o huir
	UDataTable* Markov = MakeMarkovTable(Dir, TEXT("DT_Markov_Owl"), {
		{ TEXT("Sleep"),     Row({ { A::Sleep, 0.45f }, { A::Perch, 0.30f }, { A::Relocate, 0.10f }, { A::Hunt, 0.15f } }) },
		{ TEXT("Perch"),     Row({ { A::Sleep, 0.30f }, { A::Perch, 0.20f }, { A::Relocate, 0.25f }, { A::Hunt, 0.25f } }) },
		{ TEXT("Relocate"),  Row({ { A::Sleep, 0.25f }, { A::Perch, 0.40f }, { A::Relocate, 0.10f }, { A::Hunt, 0.25f } }) },
		{ TEXT("Hunt"),      Row({ { A::Sleep, 0.40f }, { A::Perch, 0.30f }, { A::Relocate, 0.30f } }) },
		{ TEXT("Nervous"),   Row({ { A::Sleep, 0.10f }, { A::Perch, 0.20f }, { A::Relocate, 0.60f }, { A::Hunt, 0.10f } }) },
		{ TEXT("Disturbed"), Row({ { A::Attack, 0.45f }, { A::Flee, 0.55f } }) },
	}, Packages);

	// ---------- EQS ----------
	UEnvQuery* FindPerch = CreateOrLoadAsset<UEnvQuery>(Dir, TEXT("EQS_Owl_FindPerch"), Packages);
	{
		// rama alta de buho, libre y lejos del jugador
		UEnvQueryOption* Option = ResetQuery(FindPerch, ABirdPerch::StaticClass(), 4000.f);
		AddBoolTest<UEnvQueryTest_PerchAvailable>(Option, EEnvTestPurpose::Filter);
		AddBoolTest<UEnvQueryTest_OwlPerch>(Option, EEnvTestPurpose::Score);
		AddDistance(Option, UEnvQueryContext_Querier::StaticClass(), EEnvTestPurpose::Filter, 400.f, 1.f);
		AddDistance(Option, UEnvQueryContext_WingShotPlayer::StaticClass(), EEnvTestPurpose::Score, 0.f, 1.f);
	}
	UEnvQuery* FindPrey = CreateOrLoadAsset<UEnvQuery>(Dir, TEXT("EQS_Owl_FindPrey"), Packages);
	{
		// Raton vivo mas cercano
		UEnvQueryOption* Option = ResetQuery(FindPrey, AMouseCritter::StaticClass(), 5000.f);
		AddBoolTest<UEnvQueryTest_PreyAlive>(Option, EEnvTestPurpose::Filter);
		AddDistance(Option, UEnvQueryContext_Querier::StaticClass(), EEnvTestPurpose::Score, 0.f, -1.f);
	}
	UEnvQuery* FindEscape = GetEscapeQuery(Packages);

	// ---------- StateTree ----------
	UStateTree* StateTree = CreateOrLoadAsset<UStateTree>(Dir, TEXT("ST_Owl"), Packages);
	UStateTreeEditorData* EditorData = ResetStateTree(StateTree);
	AddCommonEvaluators(EditorData, true);

	UStateTreeState& Root = EditorData->AddRootState();
	UStateTreeState& Calm = Root.AddChildState(TEXT("Calm"));
	UStateTreeState& Disturbed = Root.AddChildState(TEXT("Disturbed"));

	UStateTreeState& Decide = Calm.AddChildState(TEXT("Decide"));
	UStateTreeState& Sleep = Calm.AddChildState(TEXT("Sleep"));
	UStateTreeState& Perch = Calm.AddChildState(TEXT("Perch"));
	UStateTreeState& Relocate = Calm.AddChildState(TEXT("Relocate"));
	UStateTreeState& Hunt = Calm.AddChildState(TEXT("Hunt"));

	const FGuid DecideId = AddDecide(Decide, Markov, A::Perch);
	AddActivityTransition(EditorData, Decide, DecideId, A::Sleep, Sleep);
	AddActivityTransition(EditorData, Decide, DecideId, A::Perch, Perch);
	AddActivityTransition(EditorData, Decide, DecideId, A::Relocate, Relocate);
	AddActivityTransition(EditorData, Decide, DecideId, A::Hunt, Hunt);

	{
		auto& SleepNode = Sleep.AddTask<FStateTreeTask_BirdSleep>();
		SleepNode.GetInstanceData().MinTime = 8.f;
		SleepNode.GetInstanceData().MaxTime = 15.f;
	}
	GoTo(Sleep, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	AddIdle(Perch, 4.f, 8.f);
	GoTo(Perch, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	UStateTreeState& FlyToBranch = AddSearchAndFly(EditorData, Relocate, TEXT("FlyToBranch"), FindPerch, 600.f, 200.f);
	GoTo(Relocate, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	GoTo(FlyToBranch, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	// caza, busca el raton, baja en picada siguiendolo y se lo come en el suelo
	Hunt.AddEnterCondition<FStateTreeCondition_PreyAvailable>();
	UStateTreeState& Swoop = AddSearchAndFly(EditorData, Hunt, TEXT("Swoop"), FindPrey, 900.f, 150.f, true);
	UStateTreeState& EatPrey = Hunt.AddChildState(TEXT("EatPrey"));
	GoTo(Hunt, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	GoTo(Swoop, EStateTreeTransitionTrigger::OnStateSucceeded, EatPrey);
	GoTo(Swoop, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	AddIdle(EatPrey, 4.f, 7.f);
	GoTo(EatPrey, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	// despertado, Markov (fila "Disturbed") decide atacar o huir
	Calm.AddTransition(EStateTreeTransitionTrigger::OnEvent, ABirdAIController::GetScaredEventTag(),
		EStateTreeTransitionType::GotoState, &Disturbed).Priority = EStateTreeTransitionPriority::High;
	UStateTreeState& FightOrFlight = Disturbed.AddChildState(TEXT("FightOrFlight"));
	UStateTreeState& Attack = Disturbed.AddChildState(TEXT("Attack"));
	UStateTreeState& Flee = Disturbed.AddChildState(TEXT("Flee"));

	const FGuid FightId = AddDecide(FightOrFlight, Markov, A::Flee, TEXT("Disturbed"));
	AddActivityTransition(EditorData, FightOrFlight, FightId, A::Attack, Attack);
	AddActivityTransition(EditorData, FightOrFlight, FightId, A::Flee, Flee);

	Attack.AddEnterCondition<FStateTreeCondition_ThreatInRange>().GetInstanceData().Range = 3500.f;
	{
		auto& AttackNode = Attack.AddTask<FStateTreeTask_BirdAttackPlayer>();
		AttackNode.GetInstanceData().Speed = 1000.f;
		AttackNode.GetInstanceData().Damage = 15.f;
	}
	GoTo(Attack, EStateTreeTransitionTrigger::OnStateCompleted, Flee);

	UStateTreeState& Escape = AddSearchAndFly(EditorData, Flee, TEXT("Escape"), FindEscape, 1100.f, 300.f);
	GoTo(Flee, EStateTreeTransitionTrigger::OnStateFailed, Calm);
	GoTo(Escape, EStateTreeTransitionTrigger::OnStateCompleted, Calm);

	AddStunReaction(EditorData, Root, Calm);

	return Finish(StateTree, Packages, TEXT("Buho: DT_Markov_Owl, 3 EQS"));
#else
	return TEXT("Solo disponible en el editor");
#endif
}

// Aguila, tabla Markov, EQS de ruta y presas, ST_Eagle
FString UWingShotAssetBuilder::BuildEagleAssets()
{
#if WITH_EDITOR
	using namespace WingShotBuilder;
	using A = EBirdActivity;
	TArray<UPackage*> Packages;
	const FString Dir = TEXT("/Game/WingShot/AI/Eagle");

	// ---------- Markov ----------
	UDataTable* Markov = MakeMarkovTable(Dir, TEXT("DT_Markov_Eagle"), {
		{ TEXT("Patrol"), Row({ { A::Patrol, 0.45f }, { A::Search, 0.55f } }) },
		{ TEXT("Search"), Row({ { A::Patrol, 0.30f }, { A::Search, 0.25f }, { A::Hunt, 0.35f }, { A::Attack, 0.10f } }) },
		{ TEXT("Hunt"),   Row({ { A::Patrol, 0.60f }, { A::Search, 0.40f } }) },
		{ TEXT("Attack"), Row({ { A::Patrol, 0.50f }, { A::Search, 0.50f } }) },
	}, Packages);

	// ---------- EQS ----------
	UEnvQuery* NextWaypoint = CreateOrLoadAsset<UEnvQuery>(Dir, TEXT("EQS_Eagle_NextWaypoint"), Packages);
	{
		// Siguiente punto de vuelo, no el mismo, cercano y de preferencia enfrente (vuela en circulos)
		UEnvQueryOption* Option = ResetQuery(NextWaypoint, AEagleWaypoint::StaticClass(), 8000.f);
		AddDistance(Option, UEnvQueryContext_Querier::StaticClass(), EEnvTestPurpose::FilterAndScore, 1200.f, -1.f);
		AddAheadOfQuerier(Option);
	}
	UEnvQuery* FindPrey = CreateOrLoadAsset<UEnvQuery>(Dir, TEXT("EQS_Eagle_FindPrey"), Packages);
	{
		UEnvQueryOption* Option = ResetQuery(FindPrey, AMouseCritter::StaticClass(), 4500.f);
		AddBoolTest<UEnvQueryTest_PreyAlive>(Option, EEnvTestPurpose::Filter);
		AddDistance(Option, UEnvQueryContext_Querier::StaticClass(), EEnvTestPurpose::Score, 0.f, -1.f);
	}

	// ---------- StateTree ----------
	UStateTree* StateTree = CreateOrLoadAsset<UStateTree>(Dir, TEXT("ST_Eagle"), Packages);
	UStateTreeEditorData* EditorData = ResetStateTree(StateTree);
	AddCommonEvaluators(EditorData, true);

	UStateTreeState& Root = EditorData->AddRootState();
	UStateTreeState& Sky = Root.AddChildState(TEXT("Sky"));
	UStateTreeState& Decide = Sky.AddChildState(TEXT("Decide"));
	UStateTreeState& Patrol = Sky.AddChildState(TEXT("Patrol"));
	UStateTreeState& Search = Sky.AddChildState(TEXT("Search"));
	UStateTreeState& Hunt = Sky.AddChildState(TEXT("Hunt"));
	UStateTreeState& Attack = Sky.AddChildState(TEXT("Attack"));
	UStateTreeState& Ascend = Sky.AddChildState(TEXT("Ascend"));

	const FGuid DecideId = AddDecide(Decide, Markov, A::Search);
	AddActivityTransition(EditorData, Decide, DecideId, A::Patrol, Patrol);
	AddActivityTransition(EditorData, Decide, DecideId, A::Search, Search);
	AddActivityTransition(EditorData, Decide, DecideId, A::Hunt, Hunt);
	AddActivityTransition(EditorData, Decide, DecideId, A::Attack, Attack);

	// Patrullar, vuelo rapido entre puntos. Buscar, vuelo lento observando
	UStateTreeState& Glide = AddSearchAndFly(EditorData, Patrol, TEXT("Glide"), NextWaypoint, 900.f, 150.f);
	GoTo(Patrol, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	GoTo(Glide, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	UStateTreeState& Scan = AddSearchAndFly(EditorData, Search, TEXT("Scan"), NextWaypoint, 450.f, 60.f);
	GoTo(Search, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	GoTo(Scan, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	// Si ve al jugador mientras busca o caza, puede cambiar su curso para atacarlo
	for (UStateTreeState* State : { &Search, &Hunt })
	{
		FStateTreeTransition& Spotted = State->AddTransition(EStateTreeTransitionTrigger::OnEvent,
			ABirdAIController::GetSpottedPlayerEventTag(), EStateTreeTransitionType::GotoState, &Attack);
		Spotted.Priority = EStateTreeTransitionPriority::High;
		Spotted.AddCondition<FStateTreeCondition_RandomChance>().GetInstanceData().Probability = 0.5f;
	}

	// Cazar, picada siguiendo al raton y comer en el suelo
	Hunt.AddEnterCondition<FStateTreeCondition_PreyAvailable>();
	UStateTreeState& Dive = AddSearchAndFly(EditorData, Hunt, TEXT("Dive"), FindPrey, 1400.f, 0.f, true);
	UStateTreeState& EatPrey = Hunt.AddChildState(TEXT("EatPrey"));
	GoTo(Hunt, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	GoTo(Dive, EStateTreeTransitionTrigger::OnStateSucceeded, EatPrey);
	GoTo(Dive, EStateTreeTransitionTrigger::OnStateFailed, Ascend);
	AddIdle(EatPrey, 4.f, 6.f);
	GoTo(EatPrey, EStateTreeTransitionTrigger::OnStateCompleted, Ascend);

	Attack.AddEnterCondition<FStateTreeCondition_ThreatInRange>().GetInstanceData().Range = 4500.f;
	{
		auto& AttackNode = Attack.AddTask<FStateTreeTask_BirdAttackPlayer>();
		AttackNode.GetInstanceData().Speed = 1500.f;
		AttackNode.GetInstanceData().Damage = 25.f;
	}
	GoTo(Attack, EStateTreeTransitionTrigger::OnStateCompleted, Ascend);

	// Volver a subir al cielo
	UStateTreeState& Climb = AddSearchAndFly(EditorData, Ascend, TEXT("Climb"), NextWaypoint, 900.f, 300.f);
	GoTo(Ascend, EStateTreeTransitionTrigger::OnStateFailed, Decide);
	GoTo(Climb, EStateTreeTransitionTrigger::OnStateCompleted, Decide);

	AddStunReaction(EditorData, Root, Sky);

	return Finish(StateTree, Packages, TEXT("Aguila: DT_Markov_Eagle, 2 EQS"));
#else
	return TEXT("Solo disponible en el editor");
#endif
}

// raton, ST_Mouse (caminar, descansar, huir, stun)
FString UWingShotAssetBuilder::BuildMouseAssets()
{
#if WITH_EDITOR
	using namespace WingShotBuilder;
	TArray<UPackage*> Packages;
	const FString Dir = TEXT("/Game/WingShot/AI/Mouse");

	UStateTree* StateTree = CreateOrLoadAsset<UStateTree>(Dir, TEXT("ST_Mouse"), Packages);
	UStateTreeEditorData* EditorData = ResetStateTree(StateTree);

	// Caminar al otro extremo de la ruta -> descansar -> repetir. Si percibe al jugador o a un
	// depredador, huye. Morir lo detiene el depredador
	AddCommonEvaluators(EditorData, false);
	UStateTreeState& Root = EditorData->AddRootState();
	UStateTreeState& Walk = Root.AddChildState(TEXT("Walk"));
	UStateTreeState& Rest = Root.AddChildState(TEXT("Rest"));
	UStateTreeState& Flee = Root.AddChildState(TEXT("Flee"));
	Walk.AddTask<FStateTreeTask_MouseWalkRoute>();
	GoTo(Walk, EStateTreeTransitionTrigger::OnStateCompleted, Rest);
	AddIdle(Rest, 1.5f, 4.f);
	GoTo(Rest, EStateTreeTransitionTrigger::OnStateCompleted, Walk);

	Flee.AddTask<FStateTreeTask_MouseFlee>();
	GoTo(Flee, EStateTreeTransitionTrigger::OnStateCompleted, Rest);
	Root.AddTransition(EStateTreeTransitionTrigger::OnEvent, WingShot::ScaredTag(),
		EStateTreeTransitionType::GotoState, &Flee).Priority = EStateTreeTransitionPriority::High;

	AddStunReaction(EditorData, Root, Rest);

	return Finish(StateTree, Packages, TEXT("Raton"));
#else
	return TEXT("Solo disponible en el editor");
#endif
}
