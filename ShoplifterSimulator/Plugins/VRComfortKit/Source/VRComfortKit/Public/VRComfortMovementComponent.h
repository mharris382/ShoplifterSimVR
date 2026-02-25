#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRUserProfile.h"
#include "VRComfortMovementComponent.generated.h"

class UCameraComponent;
class UMotionControllerComponent;
class UPostProcessComponent;
class UVRProfileSubsystem;

/**
 * UVRComfortMovementComponent
 *
 * Actor component that owns all VR locomotion and comfort logic.
 * Attach to your VR Pawn. It binds to UVRProfileSubsystem::OnProfileDataChanged
 * so settings apply live — no restart required.
 *
 * Expects the owning pawn to provide references to its camera and
 * motion controller components via InitializeComponents().
 *
 * This component does NOT replace UCharacterMovementComponent — it sits
 * on top and drives the pawn's movement via AddMovementInput / AddActorLocalRotation.
 * For ghost mode it directly moves the root via SetActorLocation.
 */
UCLASS(ClassGroup = "VRComfortKit", meta = (BlueprintSpawnableComponent))
class VRCOMFORTKIT_API UVRComfortMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UVRComfortMovementComponent();

	// -----------------------------------------------------------------------
	// Initialization
	// -----------------------------------------------------------------------

	/**
	 * Call from BeginPlay on your pawn after all components are initialized.
	 * LocomotionController  — the hand that drives movement (left by default).
	 * TurnController        — the hand that drives turning (right by default).
	 * Camera                — the HMD camera component.
	 * PostProcess           — optional; if nullptr, vignette will be skipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Movement")
	void InitializeComponents(
		UCameraComponent* InCamera,
		UMotionControllerComponent* InLocomotionController,
		UMotionControllerComponent* InTurnController,
		UPostProcessComponent* InPostProcess = nullptr);

	// -----------------------------------------------------------------------
	// Input — call these from your pawn's input handlers
	// -----------------------------------------------------------------------

	/** Analog stick input for locomotion. X = strafe, Y = forward. */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Movement")
	void HandleMoveInput(FVector2D Input);

	/** Analog stick input for turning. X axis used. */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Movement")
	void HandleTurnInput(FVector2D Input);

	/**
	 * For teleport mode: call when the player initiates a teleport arc.
	 * The component will handle the trace and fade internally.
	 * Returns false if not in teleport mode or trace found no valid target.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Movement")
	bool BeginTeleport();

	// -----------------------------------------------------------------------
	// Ghost / noclip
	// -----------------------------------------------------------------------

	/**
	 * Toggles ghost mode (collision disabled, free movement on all axes).
	 * Also updates the active profile via the subsystem.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Movement")
	void SetGhostMode(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "VRComfortKit|Movement")
	bool IsGhostMode() const { return bGhostModeActive; }

	// -----------------------------------------------------------------------
	// Runtime overrides (bypass profile for this session)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Movement")
	void OverrideMovementMode(EVRMovementMode Mode);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Movement")
	void ClearMovementModeOverride();

	// -----------------------------------------------------------------------
	// Accessors
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Accessors")
	EVRMovementMode GetActiveMovementMode() const
	{
		return MovementModeOverride.IsSet() ? MovementModeOverride.GetValue() : ActiveMovementMode;
	}

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Accessors")
	EVRTurnMode GetActiveTurnMode() const { return ActiveTurnMode; }

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Accessors")
	EVRDirectionMode GetActiveDirectionMode() const { return ActiveDirectionMode; }


	void TryLazyInitialize();
	// -----------------------------------------------------------------------
	// UActorComponent interface
	// -----------------------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:

	// -----------------------------------------------------------------------
	// Internal movement handlers
	// -----------------------------------------------------------------------

	void TickSmoothMovement(float DeltaTime);
	void TickFlightMovement(float DeltaTime);
	void TickSmoothTurn(float DeltaTime);
	void TickSnapTurn(float DeltaTime);
	void TickVignette(float DeltaTime);

	/** Traces downward and snaps the pawn to the floor. Called on BeginPlay and can be called manually. */
	void SnapToGround();

	/** Returns the forward vector used for locomotion based on DirectionMode.
	 *  bFlattenToHorizontal strips Z for grounded movement; pass false for flight. */
	FVector GetLocomotionForward(bool bFlattenToHorizontal) const;

	// -----------------------------------------------------------------------
	// Profile binding
	// -----------------------------------------------------------------------

	UFUNCTION()
	void OnProfileDataChanged(const FVRProfileData& NewData);

	void ApplyProfile(const FVRProfileData& Data);

	// -----------------------------------------------------------------------
	// Cached component refs (set via InitializeComponents)
	// -----------------------------------------------------------------------

	UPROPERTY()
	UCameraComponent* Camera = nullptr;

	UPROPERTY()
	UMotionControllerComponent* LocomotionController = nullptr;

	UPROPERTY()
	UMotionControllerComponent* TurnController = nullptr;

	UPROPERTY()
	UPostProcessComponent* PostProcess = nullptr;

	UPROPERTY()
	UVRProfileSubsystem* ProfileSubsystem = nullptr;

	// -----------------------------------------------------------------------
	// Active settings (mirrored from profile for fast tick access)
	// -----------------------------------------------------------------------

	EVRMovementMode ActiveMovementMode = EVRMovementMode::Teleport;
	EVRTurnMode     ActiveTurnMode = EVRTurnMode::Snap;
	EVRDirectionMode ActiveDirectionMode = EVRDirectionMode::HeadRelative;

	float ActiveMoveSpeed = 300.f;
	float ActiveSnapTurnAngle = 45.f;
	float ActiveSmoothTurnSpeed = 90.f;
	float ActiveVignetteIntensity = 0.6f;
	bool  bVignetteEnabled = true;
	bool  bHandsInverted = false;

	// -----------------------------------------------------------------------
	// Runtime state
	// -----------------------------------------------------------------------

	/** Cached move input this frame. */
	FVector2D CurrentMoveInput = FVector2D::ZeroVector;

	/** Cached turn input this frame. */
	FVector2D CurrentTurnInput = FVector2D::ZeroVector;

	/** Accumulated turn input for snap deadzone. */
	float SnapTurnAccumulator = 0.f;

	/** True while snap turn is in cooldown (prevents repeated snaps per hold). */
	bool bSnapTurnOnCooldown = false;

	bool bGhostModeActive = false;

	/** Teleport pending target — set by BeginTeleport, consumed on confirm. */
	bool bHasPendingTeleport = false;
	FVector PendingTeleportLocation = FVector::ZeroVector;

	/** Optional movement mode override (ignores profile). */
	TOptional<EVRMovementMode> MovementModeOverride;

	// -----------------------------------------------------------------------
	// Vignette post process settings names
	// -----------------------------------------------------------------------

	static const FName VignetteIntensityParamName;
};