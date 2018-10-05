#include "SGameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "SHealthComponent.h"
#include "SGameState.h"
#include "SPlayerState.h"

ASGameMode::ASGameMode() {
	TimeBetweenWaves = 2.f;

	GameStateClass = ASGameState::StaticClass();
	PlayerStateClass = ASPlayerState::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(1.f);
}

void ASGameMode::StartPlay() {
	Super::StartPlay();

	PrepareForNextWave();
}

void ASGameMode::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	
	CheckWaveState();
	CheckAnyPlayerAlive();
}

void ASGameMode::StartWave() {
	WaveCount++;
	NrOfBotsToSpawn = 2 * WaveCount;
	GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.f, true);

	SetWaveState(EWaveState::WaveInProgress);
}

void ASGameMode::CheckWaveState() {
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);
	if (NrOfBotsToSpawn > 0 || bIsPreparingForWave) { return; }

	bool bIsAnyBotAlive = false;

	for (auto It = GetWorld()->GetPawnIterator(); It; ++It) {
		auto TestPawn = It->Get();

		if (!TestPawn || TestPawn->IsPlayerControlled()) { continue; }

		auto HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.f) {
			bIsAnyBotAlive = true;
			break;
		}
	}

	if (!bIsAnyBotAlive) {
		SetWaveState(EWaveState::WaveComplete);
		PrepareForNextWave();
	}
}

void ASGameMode::CheckAnyPlayerAlive() {
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It) {
		auto PC = It->Get();

		if (PC && PC->GetPawn()) {
			auto MyPawn = PC->GetPawn();
			auto HealthComponent = Cast<USHealthComponent>(MyPawn->GetComponentByClass(USHealthComponent::StaticClass()));
			if (ensure(HealthComponent) && HealthComponent->GetHealth() > 0.f) {
				return;
			}
		}
	}
	GameOver();
}

void ASGameMode::GameOver() {
	// TODO
	SetWaveState(EWaveState::GameOver);

	UE_LOG(LogTemp, Warning, TEXT("Game Over!  Players Died"))
}

void ASGameMode::SetWaveState(EWaveState NewState) {
	auto GS = GetGameState<ASGameState>();
	if (ensureAlways(GS)) {
		GS->SetWaveState(NewState);
	}
}

void ASGameMode::PrepareForNextWave() {
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::StartWave, TimeBetweenWaves, false);
	SetWaveState(EWaveState::WaitingToStart);
}

void ASGameMode::EndWave() {
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
	SetWaveState(EWaveState::WaitingToComplete);
}

void ASGameMode::SpawnBotTimerElapsed() {
	SpawnNewBot();

	NrOfBotsToSpawn--;

	if (NrOfBotsToSpawn <= 0) {
		EndWave();
	}
}
