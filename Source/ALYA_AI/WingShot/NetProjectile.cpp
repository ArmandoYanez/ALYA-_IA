#include "WingShot/NetProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "WingShot/CatchableInterface.h"
#include "WingShot/WingShotCharacter.h"

ANetProjectile::ANetProjectile()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(30.0f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetNotifyRigidBodyCollision(true);
	RootComponent = Collision;

	// Visual de prototipo: una esfera aplanada que hace de red.
	NetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NetMesh"));
	NetMesh->SetupAttachment(Collision);
	NetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NetMesh->SetRelativeScale3D(FVector(0.15f, 0.6f, 0.6f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		NetMesh->SetStaticMesh(SphereMesh.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 2500.0f;
	Movement->MaxSpeed = 2500.0f;
	Movement->ProjectileGravityScale = 0.4f;
	Movement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 3.0f;
}

void ANetProjectile::BeginPlay()
{
	Super::BeginPlay();

	// La red no debe chocar con quien la dispara.
	if (GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(GetInstigator(), true);
	}
	Collision->OnComponentHit.AddDynamic(this, &ANetProjectile::HandleHit);
}

void ANetProjectile::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor->Implements<UCatchableInterface>() && ICatchableInterface::Execute_CanBeCaught(OtherActor))
	{
		const FName Species = ICatchableInterface::Execute_GetSpeciesName(OtherActor);
		ICatchableInterface::Execute_OnCaughtByNet(OtherActor, GetInstigator());

		if (AWingShotCharacter* Player = Cast<AWingShotCharacter>(GetInstigator()))
		{
			Player->RegisterCatch(Species);
		}
	}

	Destroy();
}
