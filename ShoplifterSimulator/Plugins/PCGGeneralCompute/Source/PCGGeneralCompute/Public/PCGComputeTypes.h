#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "PCGComputeTypes.generated.h"



USTRUCT(BlueprintType)
struct FPCGComputeQuery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCGCompute")
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCGCompute")
	float MinRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCGCompute")
	float MaxRadius = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCGCompute")
	int32 MaxResults = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCGCompute")
	int32 Seed = 12345;
};

USTRUCT(BlueprintType)
struct FPCGComputePoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PCGCompute")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "PCGCompute")
	float Score = 0.f;
};

USTRUCT(BlueprintType)
struct FPCGComputeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PCGCompute")
	TArray<FPCGComputePoint> Points;

	UPROPERTY(BlueprintReadOnly, Category = "PCGCompute")
	float ExecutionTimeMs = 0.f;
};

DECLARE_DELEGATE_OneParam(FPCGComputeCompleted, const FPCGComputeResult&);