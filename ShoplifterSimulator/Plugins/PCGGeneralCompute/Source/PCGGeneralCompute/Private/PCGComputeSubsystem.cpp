#include "PCGComputeSubsystem.h"
#include "PCGComputeRunnerActor.h"
#include "PCGRuntimeComputeKernel.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "PCGDataAsset.h"
#include "Data/PCGPointData.h"
#include "PCGContext.h" // sometimes needed depending on hooks

// If you want sync load for POC
#include "UObject/SoftObjectPtr.h"

void UPCGComputeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureRunnerExists();
}

void UPCGComputeSubsystem::Deinitialize()
{
	ActiveJob.Reset();
	bRunnerBusy = false;

	if (RunnerActor )
	{
		RunnerActor->Destroy();
	}

	RunnerActor = nullptr;
	RunnerComponent = nullptr;

	Super::Deinitialize();
}

void UPCGComputeSubsystem::SubmitQuery(UPCGRuntimeComputeKernel* Kernel, const FPCGComputeQuery& Query, FPCGComputeCompleted Completion)
{
	if (!Kernel)
	{
		return;
	}

	FPCGComputeJob Job;
	Job.JobId = FGuid::NewGuid();
	Job.Kernel = Kernel;
	Job.Query = Query;
	Job.Completion = MoveTemp(Completion);

	PendingJobs.Enqueue(MoveTemp(Job));
	TryStartNextJob();
}

void UPCGComputeSubsystem::EnsureRunnerExists()
{
	if (RunnerActor)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = false;
	Params.Name = TEXT("PCGComputeRunner_Transient");

	RunnerActor = World->SpawnActor<APCGComputeRunnerActor>(Params);
	if (!RunnerActor)
	{
		return;
	}

	RunnerComponent = RunnerActor->GetPCGComponent();

	// Bind completion callback (names/signatures vary)
	// TODO: In your UE version, search UPCGComponent for “OnGeneration”, “OnGenerated”, “OnPCGGraphGenerated”
	// Example patterns you might find:
	// RunnerComponent->OnPCGGraphGenerated.AddUObject(this, &UPCGComputeSubsystem::HandleGenerationComplete);
	// RunnerComponent->OnGenerationCompleteDelegate.AddUObject(this, &UPCGComputeSubsystem::HandleGenerationComplete);

	// For now, just ensure we have the component
}

void UPCGComputeSubsystem::TryStartNextJob()
{
	EnsureRunnerExists();
	if (!RunnerComponent || bRunnerBusy || ActiveJob.IsSet())
	{
		return;
	}

	FPCGComputeJob Next;
	if (!PendingJobs.Dequeue(Next))
	{
		return;
	}

	ActiveJob = MoveTemp(Next);
	StartJob(*ActiveJob);
}

bool UPCGComputeSubsystem::LoadKernelAssets(const UPCGRuntimeComputeKernel& Kernel, UPCGGraph*& OutGraph, UPCGDataAsset*& OutDataAsset) const
{
	OutGraph = Kernel.RuntimeGraph.LoadSynchronous();
	OutDataAsset = Kernel.CachedData.LoadSynchronous();
	return (OutGraph != nullptr) && (OutDataAsset != nullptr);
}

UPCGParamData* UPCGComputeSubsystem::BuildParamData(const FPCGComputeQuery& Query) const
{
	// ParamData = metadata-only container ideal for runtime params
	UPCGParamData* Param = NewObject<UPCGParamData>(GetTransientPackage());
	if (!Param)
	{
		return nullptr;
	}

	// TODO: Fill metadata with attributes that your graph reads.
	// The exact metadata API depends on PCG version.
	// Search for examples of UPCGParamData usage inside PCG plugin.

	// Typical intent:
	// Param->Metadata->CreateAttribute<FVector>(TEXT("Origin"), Query.Origin, /*bAllowsInterpolation*/false, /*bOverrideParent*/true);
	// Param->Metadata->CreateAttribute<float>(TEXT("MinRadius"), Query.MinRadius, false, true);
	// Param->Metadata->CreateAttribute<float>(TEXT("MaxRadius"), Query.MaxRadius, false, true);
	// Param->Metadata->CreateAttribute<int32>(TEXT("MaxResults"), Query.MaxResults, false, true);
	// Param->Metadata->CreateAttribute<int32>(TEXT("Seed"), Query.Seed, false, true);

	return Param;
}

void UPCGComputeSubsystem::StartJob(FPCGComputeJob& Job)
{
	if (!RunnerComponent)
	{
		ActiveJob.Reset();
		return;
	}

	UPCGRuntimeComputeKernel* Kernel = Job.Kernel.Get();
	if (!Kernel)
	{
		ActiveJob.Reset();
		return;
	}

	UPCGGraph* Graph = nullptr;
	UPCGDataAsset* Cached = nullptr;
	if (!LoadKernelAssets(*Kernel, Graph, Cached))
	{
		ActiveJob.Reset();
		return;
	}

	Job.StartSeconds = FPlatformTime::Seconds();
	bRunnerBusy = true;

	// 1) Configure graph
	// TODO: verify setter name in your engine version
	// RunnerComponent->SetGraph(Graph);
	// or RunnerComponent->SetPCGGraph(Graph);
	// or RunnerComponent->SetGraphInstance(Graph);

	// 2) Provide inputs:
	// - Cached data as main input (Tagged “CachedPoints” or whatever your graph expects)
	// - Param data as secondary input (Tagged “Params”)
	UPCGParamData* Params = BuildParamData(Job.Query);

	// TODO: This is the main “glue point” you’ll adapt:
	// You need to inject a custom FPCGDataCollection (tagged inputs) into the component’s next generation.
	// Search in PCG source for:
	//   - “SetInputData”
	//   - “SetExternalInput”
	//   - “FPCGDataCollection”
	//   - “UPCGComponent::GetInputData”
	// Many internal paths build an input collection from the component input type (Actor, Landscape, etc.).
	// For your compute runner, you want to override/replace that with your own collection.

	// 3) Kick generation async
	// TODO: find the right call in your version: Generate / GenerateLocal / RequestGeneration / Dirty+Generate
	// RunnerComponent->Generate(/*bForce*/true);

	// If there is a “cancel/cleanup” call, you may want to cancel any prior generation before starting.
}

void UPCGComputeSubsystem::HandleGenerationComplete(/* TODO signature */)
{
	// This should be called on game thread
	if (!ActiveJob.IsSet())
	{
		bRunnerBusy = false;
		return;
	}

	FPCGComputeJob Finished = MoveTemp(*ActiveJob);
	ActiveJob.Reset();

	FPCGComputeResult Result;
	Result.ExecutionTimeMs = float((FPlatformTime::Seconds() - Finished.StartSeconds) * 1000.0);

	if (UPCGRuntimeComputeKernel* Kernel = Finished.Kernel.Get())
	{
		ExtractResultPoints(*Kernel, Result);
	}

	bRunnerBusy = false;

	if (Finished.Completion.IsBound())
	{
		Finished.Completion.Execute(Result);
	}

	TryStartNextJob();
}

bool UPCGComputeSubsystem::ExtractResultPoints(const UPCGRuntimeComputeKernel& Kernel, FPCGComputeResult& OutResult) const
{
	if (!RunnerComponent)
	{
		return false;
	}

	// TODO: second main “glue point”:
	// You need to fetch the generated output data collection from the component after completion.
	// Search for:
	//   - “GetGeneratedOutput”
	//   - “GetOutputData”
	//   - “LastGeneratedData”
	//   - “GeneratedResources”
	//
	// Then:
	// - find the tagged output matching Kernel.OutputTag (e.g., “OutPoints”)
	// - ensure it is point data
	// - iterate points and read Location + optional score attribute

	// Pseudocode once you locate the output collection:
	// const FPCGDataCollection& Output = RunnerComponent->GetGeneratedGraphOutput();
	// TArray<FPCGTaggedData> Tagged = Output.GetInputsByPin(Kernel.OutputTag);
	// const UPCGPointData* PointData = Cast<UPCGPointData>(Tagged[0].Data);
	// for (const FPCGPoint& Pt : PointData->GetPoints()) { ... }

	return true;
}