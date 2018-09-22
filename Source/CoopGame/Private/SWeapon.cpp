// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"


// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASWeapon::Fire() {
	auto WeaponOwner = GetOwner();

	if (WeaponOwner) {
		// OutParams
		FHitResult Hit;
		FVector EyeLocation;
		FRotator EyeRotation;

		WeaponOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		FVector ShotDirection = EyeRotation.Vector();
		FVector TraceEnd = EyeLocation + ShotDirection * 10000;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(WeaponOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;

		// Tracer particle "Target" param
		FVector TracerEndPoint = TraceEnd;

		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, ECC_Visibility, QueryParams)) {
			auto HitActor = Hit.GetActor();

			TracerEndPoint = Hit.ImpactPoint;

			UGameplayStatics::ApplyPointDamage(
				HitActor,
				20.f,
				ShotDirection,
				Hit,
				WeaponOwner->GetInstigatorController(),
				this,
				DamageType
			);

			if (ImpactEffect) {
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactEffect,
					Hit.ImpactPoint,
					Hit.ImpactNormal.Rotation()
				);
			}
		}

		DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::Red, false, 2.f, 0, 2.f);

		if (MuzzleEffect) {
			UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
		}

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		if (TracerEffect) {
			auto TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
			if (TracerComp) {
				TracerComp->SetVectorParameter(TracerTargetName, TracerEndPoint);
			}
		}
	}
}
