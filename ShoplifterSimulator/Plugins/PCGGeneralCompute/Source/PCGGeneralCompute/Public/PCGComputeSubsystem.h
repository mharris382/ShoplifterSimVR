#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PCGComputeTypes.h"
#include "PCGRuntimeComputeKernel.h"
#include "PCGParamData.h"

#include "PCGComputeSubsystem.generated.h"

class APCGComputeRunnerActor;
class UPCGComponent;

USTRUCT()
struct FPCGComputeJob
{
	GENERATED_BODY()

	FGuid JobId;
	TWeakObjectPtr<UPCGRuntimeComputeKernel> Kernel;
	FPCGComputeQuery Query;
	FPCGComputeCompleted Completion;
	double StartSeconds = 0.0;
};

UCLASS()
class UPCGComputeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// POC API: one job at a time, queued
	void SubmitQuery(UPCGRuntimeComputeKernel* Kernel, const FPCGComputeQuery& Query, FPCGComputeCompleted Completion);

private:
	void EnsureRunnerExists();
	void TryStartNextJob();
	void StartJob(FPCGComputeJob& Job);

	// Called when the PCGComponent finishes generating
	void HandleGenerationComplete(/* TODO: adapt signature to your UE version */);

	// Helpers
	bool LoadKernelAssets(const UPCGRuntimeComputeKernel& Kernel, class UPCGGraph*& OutGraph, class UPCGDataAsset*& OutDataAsset) const;

	// Build param data object (metadata only) for runtime variables
	class UPCGParamData* BuildParamData(const FPCGComputeQuery& Query) const;

	// Extract point output + optional score attribute
	bool ExtractResultPoints(const UPCGRuntimeComputeKernel& Kernel, FPCGComputeResult& OutResult) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<APCGComputeRunnerActor> RunnerActor;

	UPROPERTY(Transient)
	TObjectPtr<UPCGComponent> RunnerComponent;

	TQueue<FPCGComputeJob> PendingJobs;
	TOptional<FPCGComputeJob> ActiveJob;

	bool bRunnerBusy = false;
};