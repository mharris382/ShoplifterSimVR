#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "VRUserProfile.generated.h"

UENUM(BlueprintType)
enum class EVRMovementMode : uint8
{
	Teleport    UMETA(DisplayName = "Teleport"),
	Smooth      UMETA(DisplayName = "Smooth"),
	Flight      UMETA(DisplayName = "Flight"),
};

UENUM(BlueprintType)
enum class EVRTurnMode : uint8
{
	Snap        UMETA(DisplayName = "Snap"),
	Smooth      UMETA(DisplayName = "Smooth"),
};

UENUM(BlueprintType)
enum class EVRDirectionMode : uint8
{
	HeadRelative        UMETA(DisplayName = "Head Relative"),
	ControllerRelative  UMETA(DisplayName = "Controller Relative"),
};

USTRUCT(BlueprintType)
struct FVRProfileData
{
	GENERATED_BODY()

	// Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	EVRMovementMode MovementMode = EVRMovementMode::Teleport;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	EVRDirectionMode DirectionMode = EVRDirectionMode::HeadRelative;

	// Turning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning")
	EVRTurnMode TurnMode = EVRTurnMode::Snap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning")
	float SnapTurnAngle = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning")
	float SmoothTurnSpeed = 90.f;

	// Comfort
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comfort")
	bool bVignetteEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comfort", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VignetteIntensity = 0.6f;

	// Handedness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handedness")
	bool bInvertHands = false;

	// Ghost / noclip - intentionally not persisted per profile,
	// treated as a session-only flag. Kept here for runtime convenience.
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Debug")
	bool bGhostMode = false;
};

/**
 * UVRUserProfile
 * Serialized save game object for a single named VR comfort profile.
 * Managed by UVRProfileSubsystem — do not call SaveGameToSlot directly.
 */
UCLASS(BlueprintType)
class VRCOMFORTKIT_API UVRUserProfile : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Profile")
	FString ProfileName = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	FVRProfileData Data;

	/**
	 * Resets data to defaults. If no data is provided, resets to struct-initializer defaults.
	 * UVRProfileSubsystem::ResetActiveProfile() passes in the project settings defaults automatically.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void ResetToDefaults(const FVRProfileData& InDefaults) { Data = InDefaults; }
};
