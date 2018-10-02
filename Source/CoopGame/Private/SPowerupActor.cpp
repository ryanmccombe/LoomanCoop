#include "SPowerupActor.h"
#include "TimerManager.h"


ASPowerupActor::ASPowerupActor() {
	PowerupInterval = 0.f;
	TotalNrOfTicks = 0;
}

void ASPowerupActor::BeginPlay() {
	Super::BeginPlay();
}

void ASPowerupActor::ActivatePowerup() {
	OnActivated();

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
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);
	}
}

