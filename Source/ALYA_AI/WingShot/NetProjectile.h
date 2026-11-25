#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

// Red disparada por la pistola de red. Atrapa a lo que implemente ICatchableInterface.
UCLASS()
class ALYA_AI_API ANetProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANetProjectile();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Net")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Net")
	TObjectPtr<UStaticMeshComponent> NetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Net")
	TObjectPtr<UProjectileMovementComponent> Movement;
};
