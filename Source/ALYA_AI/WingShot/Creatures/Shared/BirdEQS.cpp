#include "WingShot/Creatures/Shared/BirdEQS.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Kismet/GameplayStatics.h"
#include "WingShot/Creatures/Shared/BirdAIController.h"
#include "WingShot/World/BirdPerch.h"
#include "WingShot/Creatures/Mouse/MouseCritter.h"
#include "WingShot/Creatures/Mouse/MouseAIController.h"

namespace
{
	AActor* GetQuerierActor(const FEnvQueryInstance& QueryInstance)
	{
		return Cast<AActor>(QueryInstance.Owner.Get());
	}
}

void UEnvQueryContext_WingShotPlayer::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	if (AActor* Querier = GetQuerierActor(QueryInstance))
	{
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(Querier, 0))
		{
			// el contexto es el jugador
			UEnvQueryItemType_Actor::SetContextHelper(ContextData, Player);
		}
	}
}

void UEnvQueryContext_BirdThreat::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const APawn* Bird = Cast<APawn>(GetQuerierActor(QueryInstance));
	if (const ABirdAIController* Controller = Bird ? Cast<ABirdAIController>(Bird->GetController()) : nullptr)
	{
		// el contexto es el punto de la ultima amenaza
		UEnvQueryItemType_Point::SetContextHelper(ContextData, Controller->GetThreatLocation());
	}
}

UEnvQueryTest_PerchAvailable::UEnvQueryTest_PerchAvailable()
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_Actor::StaticClass();
	SetWorkOnFloatValues(false);
	TestPurpose = EEnvTestPurpose::Filter;
}

void UEnvQueryTest_PerchAvailable::RunTest(FEnvQueryInstance& QueryInstance) const
{
	const AActor* Querier = GetQuerierActor(QueryInstance);
	BoolValue.BindData(QueryInstance.Owner.Get(), QueryInstance.QueryID);
	const bool bWantsAvailable = BoolValue.GetValue();

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const ABirdPerch* Perch = Cast<ABirdPerch>(GetItemActor(QueryInstance, It.GetIndex()));
		// Pasa solo si la rama esta libre
		const bool bAvailable = Perch && Perch->IsAvailableFor(Querier);
		It.SetScore(TestPurpose, FilterType, bAvailable, bWantsAvailable);
	}
}

FText UEnvQueryTest_PerchAvailable::GetDescriptionTitle() const
{
	return NSLOCTEXT("WingShot", "PerchAvailableTitle", "Perch Available");
}

FText UEnvQueryTest_PerchAvailable::GetDescriptionDetails() const
{
	return NSLOCTEXT("WingShot", "PerchAvailableDetails", "descarta perchas ocupadas por otra ave");
}

UEnvQueryTest_PreyAlive::UEnvQueryTest_PreyAlive()
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_Actor::StaticClass();
	SetWorkOnFloatValues(false);
	TestPurpose = EEnvTestPurpose::Filter;
}

void UEnvQueryTest_PreyAlive::RunTest(FEnvQueryInstance& QueryInstance) const
{
	BoolValue.BindData(QueryInstance.Owner.Get(), QueryInstance.QueryID);
	const bool bWantsAlive = BoolValue.GetValue();

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const AMouseCritter* Mouse = Cast<AMouseCritter>(GetItemActor(QueryInstance, It.GetIndex()));
		// pasa solo si el raton esta vivo
		It.SetScore(TestPurpose, FilterType, Mouse && Mouse->IsAlive(), bWantsAlive);
	}
}

FText UEnvQueryTest_PreyAlive::GetDescriptionTitle() const
{
	return NSLOCTEXT("WingShot", "PreyAliveTitle", "Prey Alive");
}

FText UEnvQueryTest_PreyAlive::GetDescriptionDetails() const
{
	return NSLOCTEXT("WingShot", "PreyAliveDetails", "descarta presas muertas");
}

UEnvQueryTest_OwlPerch::UEnvQueryTest_OwlPerch()
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_Actor::StaticClass();
	SetWorkOnFloatValues(false);
	TestPurpose = EEnvTestPurpose::Score;
}

void UEnvQueryTest_OwlPerch::RunTest(FEnvQueryInstance& QueryInstance) const
{
	BoolValue.BindData(QueryInstance.Owner.Get(), QueryInstance.QueryID);
	const bool bWantsOwlPerch = BoolValue.GetValue();

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const ABirdPerch* Perch = Cast<ABirdPerch>(GetItemActor(QueryInstance, It.GetIndex()));
		// Mas puntos si es percha de buho
		It.SetScore(TestPurpose, FilterType, Perch && Perch->bOwlPerch, bWantsOwlPerch);
	}
}

FText UEnvQueryTest_OwlPerch::GetDescriptionTitle() const
{
	return NSLOCTEXT("WingShot", "OwlPerchTitle", "Owl Perch");
}

FText UEnvQueryTest_OwlPerch::GetDescriptionDetails() const
{
	return NSLOCTEXT("WingShot", "OwlPerchDetails", "prefiere perchas altas de buho");
}
