#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VRUserProfile.h"
#include "VRProfileSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVRProfileChanged, const FVRProfileData&, NewData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVRProfileLoaded, const FString&, ProfileName, bool, bSuccess);

/**
 * UVRProfileSubsystem
 *
 * GameInstance subsystem that owns the active VR comfort profile.
 * Provides load/save/switch operations and broadcasts changes so
 * the movement component and any UI can react without polling.
 *
 * Usage:
 *   UVRProfileSubsystem* PS = GetGameInstance()->GetSubsystem<UVRProfileSubsystem>();
 *   PS->LoadProfile("Default");
 *   PS->GetActiveData().MoveSpeed = 400.f;
 *   PS->SaveActiveProfile();
 */
UCLASS()
class VRCOMFORTKIT_API UVRProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// USubsystem interface
	// -----------------------------------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------------
	// Profile operations
	// -----------------------------------------------------------------------

	/**
	 * Loads a profile by name from disk. Falls back to creating a fresh
	 * Default profile if the slot doesn't exist. Broadcasts OnProfileLoaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void LoadProfile(const FString& ProfileName);

	/**
	 * Saves the currently active profile to its slot.
	 * Returns false if no profile is active.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	bool SaveActiveProfile();

	/**
	 * Saves the active profile then loads a different one.
	 * Convenient for a profile-select menu.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SwitchProfile(const FString& NewProfileName);

	/**
	 * Resets the active profile data to defaults and optionally saves.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void ResetActiveProfile(bool bSaveAfterReset = true);

	/**
	 * Deletes a saved profile slot from disk.
	 * Cannot delete the currently active profile.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	bool DeleteProfile(const FString& ProfileName);

	// -----------------------------------------------------------------------
	// Data accessors
	// -----------------------------------------------------------------------

	/** Returns a copy of the active profile data. Use SetActiveData to write. */
	UFUNCTION(BlueprintPure, Category = "VRComfortKit|Profile")
	FVRProfileData GetActiveData() const;

	/**
	 * Writes a full data struct to the active profile and broadcasts the
	 * change delegate. Does NOT automatically save to disk.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetActiveData(const FVRProfileData& NewData);

	/** Convenience — set a single movement mode and broadcast. */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetMovementMode(EVRMovementMode Mode);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetTurnMode(EVRTurnMode Mode);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetMoveSpeed(float Speed);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetSnapTurnAngle(float Degrees);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetSmoothTurnSpeed(float DegreesPerSecond);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetVignetteEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetVignetteIntensity(float Intensity);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetInvertHands(bool bInvert);

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetDirectionMode(EVRDirectionMode Mode);

	/** Session-only ghost mode — never persisted. */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Profile")
	void SetGhostMode(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "VRComfortKit|Profile")
	FString GetActiveProfileName() const;

	UFUNCTION(BlueprintPure, Category = "VRComfortKit|Profile")
	bool HasActiveProfile() const { return ActiveProfile != nullptr; }

	// -----------------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------------

	/** Fired whenever any field in the active profile data changes. */
	UPROPERTY(BlueprintAssignable, Category = "VRComfortKit|Profile")
	FOnVRProfileChanged OnProfileDataChanged;

	/** Fired after a LoadProfile call completes (success or fallback). */
	UPROPERTY(BlueprintAssignable, Category = "VRComfortKit|Profile")
	FOnVRProfileLoaded OnProfileLoaded;

	// -----------------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------------

	static const FString DefaultProfileName;
	static const int32 UserIndex;

private:

	UPROPERTY()
	UVRUserProfile* ActiveProfile = nullptr;

	/** Internal helper — broadcasts OnProfileDataChanged. */
	void BroadcastDataChanged();

	/** Returns the default FVRProfileData from project settings, or engine defaults if settings unavailable. */
	static FVRProfileData GetProjectDefaultData();

	/** Builds the save slot string from a profile name. */
	static FString MakeSlotName(const FString& ProfileName);
};
