#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PCGRuntimeComputeKernel.generated.h"

class UPCGGraph;
class UPCGDataAsset;

UCLASS(BlueprintType)
class UPCGRuntimeComputeKernel : public UDataAsset
{
	GENERATED_BODY()
public:
	// Runtime compute graph: point-in -> point-out (no spawners)
	UPROPERTY(EditAnywhere, Category = "Kernel")
	TSoftObjectPtr<UPCGGraph> RuntimeGraph;

	// Cached baked data asset containing point data (or any PCG data you want)
	UPROPERTY(EditAnywhere, Category = "Kernel")
	TSoftObjectPtr<UPCGDataAsset> CachedData;

	// Output pin/tag name you expect (keep this consistent)
	UPROPERTY(EditAnywhere, Category = "Kernel")
	FName OutputTag = TEXT("OutPoints");

	// Optional score attribute name (graph writes it)
	UPROPERTY(EditAnywhere, Category = "Kernel")
	FName ScoreAttribute = TEXT("Score");
};