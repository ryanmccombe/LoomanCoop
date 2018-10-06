// Fill out your copyright notice in the Description page of Project Settings.

#include "SHealthComponent.h"
#include "UnrealNetwork.h"
#include "Engine/World.h"
#include "SGameMode.h"


// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{
	DefaultHealth = 100.f;
	TeamNum = 255;

	SetIsReplicated(true);
	bIsDead = false;
}


bool USHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB) {
	if (ActorA == nullptr || ActorB == nullptr) return true;

	auto HealthComponentA = Cast<USHealthComponent>(ActorA->GetComponentByClass(USHealthComponent::StaticClass()));
	auto HealthComponentB = Cast<USHealthComponent>(ActorB->GetComponentByClass(USHealthComponent::StaticClass()));

	if (HealthComponentA == nullptr || HealthComponentB == nullptr) return true;

	return HealthComponentA->TeamNum == HealthComponentB->TeamNum;
}

// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority) {
		auto MyOwner = GetOwner();

		if (MyOwner) {
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
		}
	}

	Health = DefaultHealth;
}

float USHealthComponent::GetHealth() const { return Health; }

void USHealthComponent::OnRep_Health(float OldHealth) {
	float Damage = Health - OldHealth;
	OnHealthChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}

void USHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser) {
	if (Damage <= 0.f || bIsDead) return;
	if (DamageCauser != DamagedActor && IsFriendly(DamagedActor, DamageCauser)) return;

	Health = FMath::Clamp(Health - Damage, 0.f, DefaultHealth);
	bIsDead = Health <= 0.f;

	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

	if (bIsDead) {
		auto GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
		if (GM) {
			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
}

void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USHealthComponent, Health);
}

void USHealthComponent::Heal(float HealAmount) {
	if (HealAmount <= 0.f || Health <= 0.f) { return; }

	Health = FMath::Clamp(Health + HealAmount, 0.f, DefaultHealth);

	OnHealthChanged.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr);

	UE_LOG(LogTemp, Warning, TEXT("Health Changed: %s (+%s)"), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(HealAmount))
}
