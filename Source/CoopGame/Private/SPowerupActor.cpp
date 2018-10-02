#include "SPowerupActor.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"

ASPowerupActor::ASPowerupActor() {
	PowerupInterval = 0.f;
	TotalNrOfTicks = 0;
	bIsPowerupActive = false;

	SetReplicates(true);
}

void ASPowerupActor::ActivatePowerup(AActor* ActivateFor) {
	OnActivated(ActivateFor);

	bIsPowerupActive = true;
	OnRep_PowerupActive();

	if (PowerupInterval > 0.f) {
		GetWorldTimerManager().SetTimer(TimerHandle_PowerupTick, this, &ASPowerupActor::OnTickPowerup, PowerupInterval, true);
	} else {
		OnTickPowerup();
	}
}

void ASPowerupActor::OnTickPowerup() {
	TicksProcessed++;

	OnPowerupTicked();
	
	if (TicksProcessed >= TotalNrOfTicks) {
		OnExpired();

		bIsPowerupActive = false;
		OnRep_PowerupActive();

		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);
	}
}

void ASPowerupActor::OnRep_PowerupActive() {
	OnPowerupStateChange(bIsPowerupActive);
}

void ASPowerupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPowerupActor, bIsPowerupActive);
}
