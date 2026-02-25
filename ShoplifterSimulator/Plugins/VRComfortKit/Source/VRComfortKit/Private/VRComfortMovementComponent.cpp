#include "VRComfortMovementComponent.h"
#include "VRProfileSubsystem.h"
#include "VRComfortKitSettings.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "DrawDebugHelpers.h"

const FName UVRComfortMovementComponent::VignetteIntensityParamName = TEXT("VignetteIntensity");

namespace
{
	void LogMessage(const FString& Message, bool bPrintToScreen)
	{
		UE_LOG(LogTemp, Log, TEXT("VRComfortMovementComponent: %s"), *Message);
		if (bPrintToScreen && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
				FString::Printf(TEXT("VRComfortMovementComponent: %s"), *Message));
		}
	}
}

UVRComfortMovementComponent::UVRComfortMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UVRComfortMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		ProfileSubsystem = GI->GetSubsystem<UVRProfileSubsystem>();
		if (ProfileSubsystem)
		{
			ProfileSubsystem->OnProfileDataChanged.AddDynamic(
				this, &UVRComfortMovementComponent::OnProfileDataChanged);
			ApplyProfile(ProfileSubsystem->GetActiveData());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VRComfortMovementComponent: UVRProfileSubsystem not found."));
		}
	}

	SnapToGround();
}

void UVRComfortMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ProfileSubsystem)
	{
		ProfileSubsystem->OnProfileDataChanged.RemoveDynamic(
			this, &UVRComfortMovementComponent::OnProfileDataChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UVRComfortMovementComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TryLazyInitialize();

	// --- CHECKPOINT 1: Camera ---
	if (!Camera)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Red, TEXT("[VRComfort] FAIL CP1: Camera is null — TryLazyInit did not find a camera"));
		return;
	}
	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green, TEXT("[VRComfort] CP1 OK: Camera valid"));

	// --- CHECKPOINT 2: Move input ---
	GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Yellow,
		FString::Printf(TEXT("[VRComfort] CP2: MoveInput=(%0.2f, %0.2f) TurnInput=(%0.2f, %0.2f)"),
			CurrentMoveInput.X, CurrentMoveInput.Y, CurrentTurnInput.X, CurrentTurnInput.Y));

	// --- CHECKPOINT 3: Resolved movement mode ---
	const EVRMovementMode ResolvedMode = MovementModeOverride.IsSet()
		? MovementModeOverride.GetValue() : ActiveMovementMode;

	const FString ModeStr = (ResolvedMode == EVRMovementMode::Smooth) ? TEXT("Smooth")
		: (ResolvedMode == EVRMovementMode::Flight) ? TEXT("Flight") : TEXT("Teleport");
	GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Yellow,
		FString::Printf(TEXT("[VRComfort] CP3: ResolvedMode=%s GhostMode=%d"), *ModeStr, (int)bGhostModeActive));

	if (bGhostModeActive)
	{
		TickFlightMovement(DeltaTime);
	}
	else
	{
		switch (ResolvedMode)
		{
		case EVRMovementMode::Smooth:   TickSmoothMovement(DeltaTime); break;
		case EVRMovementMode::Flight:   TickFlightMovement(DeltaTime); break;
		case EVRMovementMode::Teleport:
			GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Orange, TEXT("[VRComfort] CP4: Teleport mode — no smooth tick"));
			break;
		}
	}

	switch (ActiveTurnMode)
	{
	case EVRTurnMode::Smooth: TickSmoothTurn(DeltaTime); break;
	case EVRTurnMode::Snap:   TickSnapTurn(DeltaTime);   break;
	}

	if (ResolvedMode != EVRMovementMode::Teleport && !bGhostModeActive)
		TickVignette(DeltaTime);
	else if (PostProcess && bVignetteEnabled)
		PostProcess->Settings.VignetteIntensity = 0.f;

	CurrentMoveInput = FVector2D::ZeroVector;
	CurrentTurnInput = FVector2D::ZeroVector;
}

void UVRComfortMovementComponent::TryLazyInitialize()
{
	if (Camera) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	Camera = Owner->FindComponentByClass<UCameraComponent>();

	TArray<UMotionControllerComponent*> Controllers;
	Owner->GetComponents<UMotionControllerComponent>(Controllers);

	for (UMotionControllerComponent* MC : Controllers)
	{
		const FName Src = MC->GetTrackingMotionSource();
		if (Src == FName("Left") || Src == FName("LeftGrip") || Src == FName("LeftAim"))
		{
			if (!LocomotionController) LocomotionController = MC;
		}
		else if (Src == FName("Right") || Src == FName("RightGrip") || Src == FName("RightAim"))
		{
			if (!TurnController) TurnController = MC;
		}
	}

	if (bHandsInverted) Swap(LocomotionController, TurnController);

	if (Camera)
		LogMessage(TEXT("LazyInit succeeded."), true);
	else
		UE_LOG(LogTemp, Warning, TEXT("VRComfortMovementComponent: LazyInit failed — no CameraComponent found on %s"), *Owner->GetName());
}

void UVRComfortMovementComponent::InitializeComponents(
	UCameraComponent* InCamera,
	UMotionControllerComponent* InLocomotionController,
	UMotionControllerComponent* InTurnController,
	UPostProcessComponent* InPostProcess)
{
	Camera = InCamera;
	LocomotionController = InLocomotionController;
	TurnController = InTurnController;
	PostProcess = InPostProcess;
}

void UVRComfortMovementComponent::HandleMoveInput(FVector2D Input)
{
	TryLazyInitialize();
	CurrentMoveInput = Input;
}

void UVRComfortMovementComponent::HandleTurnInput(FVector2D Input)
{
	TryLazyInitialize();
	CurrentTurnInput = Input;
}

bool UVRComfortMovementComponent::BeginTeleport()
{
	const EVRMovementMode ResolvedMode = MovementModeOverride.IsSet()
		? MovementModeOverride.GetValue() : ActiveMovementMode;

	if (ResolvedMode != EVRMovementMode::Teleport || bGhostModeActive) return false;
	if (!Camera || !GetOwner()) return false;

	const USceneComponent* AimSource = LocomotionController
		? static_cast<USceneComponent*>(LocomotionController)
		: static_cast<USceneComponent*>(Camera);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	const FVector TraceStart = AimSource->GetComponentLocation();
	const FVector TraceEnd = TraceStart + AimSource->GetForwardVector() * 5000.f;

	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params)
		|| !Hit.bBlockingHit) return false;

	if (FVector::DotProduct(Hit.Normal, FVector::UpVector) < 0.7f) return false;

	PendingTeleportLocation = Hit.Location;
	bHasPendingTeleport = true;

	if (const ACharacter* Char = Cast<ACharacter>(GetOwner()))
		PendingTeleportLocation.Z += Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	GetOwner()->SetActorLocation(PendingTeleportLocation, false, nullptr, ETeleportType::TeleportPhysics);
	bHasPendingTeleport = false;
	return true;
}

void UVRComfortMovementComponent::SetGhostMode(bool bEnabled)
{
	bGhostModeActive = bEnabled;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (Prim->IsCollisionEnabled() || bEnabled)
		{
			Prim->SetCollisionEnabled(
				bEnabled ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		}
	}

	if (ACharacter* Char = Cast<ACharacter>(Owner))
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			CMC->GravityScale = bEnabled ? 0.f : 1.f;
			CMC->SetMovementMode(bEnabled ? MOVE_Flying : MOVE_Walking);
		}
	}

	if (ProfileSubsystem) ProfileSubsystem->SetGhostMode(bEnabled);
}

void UVRComfortMovementComponent::OverrideMovementMode(EVRMovementMode Mode) { MovementModeOverride = Mode; }
void UVRComfortMovementComponent::ClearMovementModeOverride() { MovementModeOverride.Reset(); }

void UVRComfortMovementComponent::OnProfileDataChanged(const FVRProfileData& NewData) { ApplyProfile(NewData); }

void UVRComfortMovementComponent::ApplyProfile(const FVRProfileData& Data)
{
	ActiveMovementMode = Data.MovementMode;
	ActiveTurnMode = Data.TurnMode;
	ActiveDirectionMode = Data.DirectionMode;
	ActiveMoveSpeed = Data.MoveSpeed;
	ActiveSnapTurnAngle = Data.SnapTurnAngle;
	ActiveSmoothTurnSpeed = Data.SmoothTurnSpeed;
	bVignetteEnabled = Data.bVignetteEnabled;
	ActiveVignetteIntensity = Data.VignetteIntensity;
	bHandsInverted = Data.bInvertHands;

	if (Data.bGhostMode != bGhostModeActive) SetGhostMode(Data.bGhostMode);

	bSnapTurnOnCooldown = false;
	SnapTurnAccumulator = 0.f;
}

void UVRComfortMovementComponent::SnapToGround()
{
	TryLazyInitialize();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	const FVector Loc = Owner->GetActorLocation();
	if (GetWorld()->LineTraceSingleByChannel(Hit,
		Loc + FVector(0, 0, 50), Loc - FVector(0, 0, 500), ECC_Visibility, Params))
	{
		Owner->SetActorLocation(FVector(Loc.X, Loc.Y, Hit.Location.Z),
			false, nullptr, ETeleportType::TeleportPhysics);
	}
}

FVector UVRComfortMovementComponent::GetLocomotionForward(bool bFlattenToHorizontal) const
{
	const USceneComponent* Src = nullptr;

	switch (ActiveDirectionMode)
	{
	case EVRDirectionMode::HeadRelative:
		Src = Camera;
		break;
	case EVRDirectionMode::ControllerRelative:
		Src = bHandsInverted ? TurnController : LocomotionController;
		if (!Src) Src = Camera;
		break;
	}

	if (!Src) return FVector::ForwardVector;

	FVector Forward = Src->GetForwardVector();
	if (bFlattenToHorizontal) Forward.Z = 0.f;

	return Forward.IsNearlyZero() ? FVector::ForwardVector : Forward.GetSafeNormal();
}

void UVRComfortMovementComponent::TickSmoothMovement(float DeltaTime)
{
	// --- CHECKPOINT 5: Entered TickSmoothMovement ---
	GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Cyan, TEXT("[VRComfort] CP5: Entered TickSmoothMovement"));

	if (CurrentMoveInput.IsNearlyZero())
	{
		GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Orange, TEXT("[VRComfort] CP6: EARLY OUT — MoveInput is zero"));
		return;
	}
	GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Cyan,
		FString::Printf(TEXT("[VRComfort] CP6: MoveInput non-zero (%0.2f, %0.2f)"), CurrentMoveInput.X, CurrentMoveInput.Y));

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		GEngine->AddOnScreenDebugMessage(7, 0.f, FColor::Red, TEXT("[VRComfort] CP7: FAIL — Owner is null"));
		return;
	}

	// --- CHECKPOINT 7: Forward vector ---
	const FVector Forward = GetLocomotionForward(true);
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	GEngine->AddOnScreenDebugMessage(7, 0.f, FColor::Cyan,
		FString::Printf(TEXT("[VRComfort] CP7: Forward=%s Right=%s"), *Forward.ToString(), *Right.ToString()));

	// --- CHECKPOINT 8: Delta ---
	const FVector HorizDelta =
		(Forward * CurrentMoveInput.Y + Right * CurrentMoveInput.X) * ActiveMoveSpeed * DeltaTime;
	GEngine->AddOnScreenDebugMessage(8, 0.f, FColor::Cyan,
		FString::Printf(TEXT("[VRComfort] CP8: MoveSpeed=%0.1f Delta=%s"), ActiveMoveSpeed, *HorizDelta.ToString()));

	FVector NewLoc = Owner->GetActorLocation() + FVector(HorizDelta.X, HorizDelta.Y, 0.f);

	// Debug arrows — forward direction and intended movement delta
	const FVector OwnerLoc = Owner->GetActorLocation();
	DrawDebugDirectionalArrow(GetWorld(), OwnerLoc + FVector(0, 0, 10), OwnerLoc + FVector(0, 0, 10) + Forward * 50.f,
		10.f, FColor::Blue, false, -1.f, 0, 2.f);
	DrawDebugDirectionalArrow(GetWorld(), OwnerLoc + FVector(0, 0, 10), OwnerLoc + FVector(0, 0, 10) + HorizDelta * 20.f,
		10.f, FColor::Green, false, -1.f, 0, 2.f);

	// --- CHECKPOINT 9: Floor trace ---
	FHitResult FloorHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	const bool bFloorHit = GetWorld()->LineTraceSingleByChannel(FloorHit,
		NewLoc + FVector(0, 0, 50), NewLoc - FVector(0, 0, 200), ECC_Visibility, Params);

	if (bFloorHit)
	{
		NewLoc.Z = FloorHit.Location.Z;
		GEngine->AddOnScreenDebugMessage(9, 0.f, FColor::Cyan,
			FString::Printf(TEXT("[VRComfort] CP9: Floor hit at Z=%0.1f"), FloorHit.Location.Z));
	}
	else
	{
		NewLoc.Z = Owner->GetActorLocation().Z;
		GEngine->AddOnScreenDebugMessage(9, 0.f, FColor::Orange, TEXT("[VRComfort] CP9: No floor hit — preserving Z"));
	}

	// --- CHECKPOINT 10: SetActorLocation ---
	const FVector OldLoc = Owner->GetActorLocation();
	bool bMoved = Owner->SetActorLocation(NewLoc, true, nullptr, ETeleportType::None);
	const FVector ActualNewLoc = Owner->GetActorLocation();

	GEngine->AddOnScreenDebugMessage(10, 0.f, bMoved ? FColor::Green : FColor::Red,
		FString::Printf(TEXT("[VRComfort] CP10: SetActorLocation result=%d OldLoc=%s NewLoc=%s ActualLoc=%s"),
			(int)bMoved, *OldLoc.ToString(), *NewLoc.ToString(), *ActualNewLoc.ToString()));
}

void UVRComfortMovementComponent::TickFlightMovement(float DeltaTime)
{
	if (CurrentMoveInput.IsNearlyZero()) return;
	if (!Camera) return;

	const FVector Forward = GetLocomotionForward(false);
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	const FVector Delta = (Forward * CurrentMoveInput.Y + Right * CurrentMoveInput.X) * ActiveMoveSpeed * DeltaTime;

	GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() + Delta,
		bGhostModeActive ? false : true, nullptr, ETeleportType::None);
}

void UVRComfortMovementComponent::TickSmoothTurn(float DeltaTime)
{
	if (FMath::IsNearlyZero(CurrentTurnInput.X)) return;
	GetOwner()->AddActorWorldRotation(FRotator(0.f, CurrentTurnInput.X * ActiveSmoothTurnSpeed * DeltaTime, 0.f));
}

void UVRComfortMovementComponent::TickSnapTurn(float DeltaTime)
{
	const float Input = CurrentTurnInput.X;
	if (FMath::Abs(Input) < 0.5f)
	{
		bSnapTurnOnCooldown = false;
		SnapTurnAccumulator = 0.f;
		return;
	}
	if (bSnapTurnOnCooldown) return;
	GetOwner()->AddActorWorldRotation(FRotator(0.f, ActiveSnapTurnAngle * (Input > 0.f ? 1.f : -1.f), 0.f));
	bSnapTurnOnCooldown = true;
}

void UVRComfortMovementComponent::TickVignette(float DeltaTime)
{
	if (!PostProcess || !bVignetteEnabled)
	{
		if (PostProcess) PostProcess->Settings.VignetteIntensity = 0.f;
		return;
	}

	const float Target = FMath::Clamp(CurrentMoveInput.Size(), 0.f, 1.f) * ActiveVignetteIntensity;
	PostProcess->Settings.VignetteIntensity =
		FMath::FInterpTo(PostProcess->Settings.VignetteIntensity, Target, DeltaTime, 8.f);
}