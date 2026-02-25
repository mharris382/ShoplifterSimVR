#include "PCGComputeRunnerActor.h"
#include "GameFramework/Actor.h"
#include "PCGComponent.h" // from PCG plugin

APCGComputeRunnerActor::APCGComputeRunnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SetActorHiddenInGame(true);
	SetCanBeDamaged(false);

	// Create a simple scene root
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Create PCG component (non-scene)
	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));

	// Register with actor — no attachment needed since it’s not scene-based
	AddOwnedComponent(PCGComponent);


	// Strongly recommended for a compute runner
	// TODO: adjust to your UE version if names differ
	// PCGComponent->bActivated = true;
	// PCGComponent->SetAutoActivate(true);
	// PCGComponent->SetIsComponentTickEnabled(false); // the PCG system schedules work; your subsystem controls job flow
}