#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCGComputeRunnerActor.generated.h"

class UPCGComponent;
class USceneComponent;

UCLASS(NotPlaceable, Transient)
class APCGComputeRunnerActor : public AActor
{
	GENERATED_BODY()

public:
	APCGComputeRunnerActor();

	UPCGComponent* GetPCGComponent() const { return PCGComponent; }

private:
	// Dummy root so the actor is valid in the world
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	// The actual PCG component
	UPROPERTY()
	TObjectPtr<UPCGComponent> PCGComponent;
};