#include "STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "SHealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SphereComponent.h"
#include "SCharacter.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

// Sets default values
ASTrackerBot::ASTrackerBot()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);

	bUseVelocityChange = false;
	MovementForce = 1000;
	RequiredDistanceToTarget = 100;

	ExplosionDamage = 40;
	ExplosionRadius = 200;
	SelfDamageInterval = 0.25f;
}

void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority) {
		NextPathPoint = GetNextPathPoint();

		FTimerHandle TimerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearbyBots, 1.f, true);
	}
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComponent, float Health, float HealthDelta,
	const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser) {

	// Pulse material on hit
	if (MatInst == nullptr) {
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst) {
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	// Explode if health is 0
	if (Health <= 0.f) {
		SelfDestruct();
	}
	
}

FVector ASTrackerBot::GetNextPathPoint() {
	auto PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);

	auto NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);

	if (NavPath && NavPath->PathPoints.Num() > 1) {
		return NavPath->PathPoints[1];
	}

	return GetActorLocation();
}

void ASTrackerBot::SelfDestruct() {
	if (bExploded) { return; }

	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffects, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	MeshComp->SetVisibility(false, true);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (Role == ROLE_Authority) {
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float Damage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		UGameplayStatics::ApplyRadialDamage(this, Damage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.f, 0, 1.f);

		SetLifeSpan(2.f);
	}
}



void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role != ROLE_Authority || bExploded) { return; }

	auto DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

	if (DistanceToTarget <= RequiredDistanceToTarget) {
		NextPathPoint = GetNextPathPoint();
		DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached");
	} else {
		FVector ForceDirection = NextPathPoint - GetActorLocation();
		ForceDirection.Normalize();
		ForceDirection *= MovementForce;

		MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor:: Yellow, false, 0.f, 0, 1.f);
	}

	DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 0.f, 1.f);
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor) {
	Super::NotifyActorBeginOverlap(OtherActor);

	if (bStartedSelfDestruction || bExploded) { return; }
	auto PlayerPawn = Cast<ASCharacter>(OtherActor);

	if (PlayerPawn) {
		if (Role == ROLE_Authority) {
			GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.f);
		}
		
		bStartedSelfDestruction = true;
		UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
	}
}

void ASTrackerBot::DamageSelf() {
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::OnCheckNearbyBots() {
	const float Radius = 600;

	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	FCollisionObjectQueryParams QueryParams;

	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;

	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);

	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.f);

	int32 NrOfBots = 0;

	for (auto Result : Overlaps) {
		auto Bot = Cast<ASTrackerBot>(Result.GetActor());
		if (Bot && Bot != this) {
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;

	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);
	UE_LOG(LogTemp, Warning, TEXT("Power Level: %i, Overlaps: %i, NrBots: %i"), PowerLevel, Overlaps.Num(), NrOfBots)
	

	if (MatInst == nullptr) {
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst) {
		float Alpha = PowerLevel / float(MaxPowerLevel);
		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}
}