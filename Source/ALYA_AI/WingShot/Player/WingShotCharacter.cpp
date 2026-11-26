#include "WingShot/Player/WingShotCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "Perception/AISense_Hearing.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AWingShotCharacter::AWingShotCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// acciones de input por defecto (creadas por Tools/setup_player.py)
	static ConstructorHelpers::FObjectFinder<UInputAction> SneakFinder(TEXT("/Game/WingShot/Input/IA_Sneak.IA_Sneak"));
	static ConstructorHelpers::FObjectFinder<UInputAction> BinocularsFinder(TEXT("/Game/WingShot/Input/IA_Binoculars.IA_Binoculars"));
	static ConstructorHelpers::FObjectFinder<UInputAction> FireNetFinder(TEXT("/Game/WingShot/Input/IA_FireNet.IA_FireNet"));
	SneakAction = SneakFinder.Object;
	BinocularsAction = BinocularsFinder.Object;
	FireNetAction = FireNetFinder.Object;
}

void AWingShotCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	Health = MaxHealth;
	DefaultFOV = GetFirstPersonCameraComponent()->FieldOfView;
}

// Conecta las teclas, Ctrl = sigilo, clic derecho = binoculares, clic izquierdo = red
void AWingShotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (SneakAction)
		{
			Input->BindAction(SneakAction, ETriggerEvent::Started, this, &AWingShotCharacter::StartSneak);
			Input->BindAction(SneakAction, ETriggerEvent::Completed, this, &AWingShotCharacter::StopSneak);
		}
		if (BinocularsAction)
		{
			Input->BindAction(BinocularsAction, ETriggerEvent::Started, this, &AWingShotCharacter::StartBinoculars);
			Input->BindAction(BinocularsAction, ETriggerEvent::Completed, this, &AWingShotCharacter::StopBinoculars);
		}
		if (FireNetAction)
		{
			Input->BindAction(FireNetAction, ETriggerEvent::Started, this, &AWingShotCharacter::FireNet);
		}
	}
}

void AWingShotCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Zoom suave de los binoculares
	UCameraComponent* Camera = GetFirstPersonCameraComponent();
	const float TargetFOV = bUsingBinoculars ? BinocularsFOV : DefaultFOV;
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, ZoomInterpSpeed));

	UpdateFootstepNoise(DeltaSeconds);
}

void AWingShotCharacter::DoAim(float Yaw, float Pitch)
{
	// Con los binoculares la camara se mueve mas lento para poder apuntar con precision
	const float Scale = bUsingBinoculars ? BinocularsLookScale : 1.0f;
	Super::DoAim(Yaw * Scale, Pitch * Scale);
}

// Sigilo, camina lento y hace menos ruido
void AWingShotCharacter::StartSneak()
{
	bIsSneaking = true;
	GetCharacterMovement()->MaxWalkSpeed = SneakSpeed;
}

void AWingShotCharacter::StopSneak()
{
	bIsSneaking = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

// Binoculares, oculta los brazos; el zoom se hace en Tick
void AWingShotCharacter::StartBinoculars()
{
	bUsingBinoculars = true;
	GetFirstPersonMesh()->SetVisibility(false, true);
}

void AWingShotCharacter::StopBinoculars()
{
	bUsingBinoculars = false;
	GetFirstPersonMesh()->SetVisibility(true, true);
}

void AWingShotCharacter::UpdateFootstepNoise(float DeltaSeconds)
{
	const bool bWalking = GetCharacterMovement()->IsMovingOnGround() && GetVelocity().Size2D() > 50.0f;
	if (!bWalking)
	{
		FootstepTimer = 0.0f;
		return;
	}

	FootstepTimer += DeltaSeconds;
	if (FootstepTimer < FootstepInterval)
	{
		return;
	}
	FootstepTimer = 0.0f;

	// Los pasos sigilosos se oyen mucho mas cerca, asi el jugador puede acercarse a las aves
	const float Range = bIsSneaking ? SneakNoiseRange : WalkNoiseRange;
	const float Loudness = bIsSneaking ? 0.2f : 1.0f;
	// ruido que las criaturas oyen con AI Perception
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), Loudness, this, Range, TEXT("Footstep"));
}

void AWingShotCharacter::FireNet()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (!NetClass || Now - LastNetTime < NetCooldown)
	{
		return;
	}
	LastNetTime = Now;

	const UCameraComponent* Camera = GetFirstPersonCameraComponent();
	const FRotator AimRotation = GetControlRotation();
	const FVector SpawnLocation = Camera->GetComponentLocation() + AimRotation.Vector() * 80.0f;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Dispara la red desde la camara
	GetWorld()->SpawnActor<AActor>(NetClass, SpawnLocation, AimRotation, Params);

	// disparar hace ruido, las aves cercanas lo escuchan
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 2000.0f, TEXT("NetShot"));
}

// suma una captura (el numero del HUD)
void AWingShotCharacter::RegisterCatch(FName Species)
{
	CaughtBySpecies.FindOrAdd(Species)++;
	TotalCaught++;

	UE_LOG(LogTemp, Display, TEXT("Atrapaste: %s (total: %d)"), *Species.ToString(), TotalCaught);
}

float AWingShotCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	// Baja la vida (ataques de buho y aguila)
	Health = FMath::Max(0.f, Health - DamageAmount);

	// empujon en direccion contraria al atacante
	if (DamageCauser)
	{
		const FVector Push = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D() * 600.f + FVector(0.f, 0.f, 250.f);
		LaunchCharacter(Push, true, true);
	}

	UE_LOG(LogTemp, Display, TEXT("%s ataco al jugador (-%.0f)"), *GetNameSafe(DamageCauser), DamageAmount);

	// Noqueado, reaparece en el inicio con la salud completa
	if (Health <= 0.f)
	{
		if (const AActor* Start = UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass()))
		{
			SetActorLocation(Start->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		}
		Health = MaxHealth;
		UE_LOG(LogTemp, Display, TEXT("Jugador noqueado: reaparece en el claro."));
	}
	return Applied;
}
