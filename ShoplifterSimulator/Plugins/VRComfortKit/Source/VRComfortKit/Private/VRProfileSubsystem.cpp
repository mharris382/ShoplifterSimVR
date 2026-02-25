#include "VRProfileSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "VRUserProfile.h"
#include "VRComfortKitSettings.h"

// -----------------------------------------------------------------------
// Statics
// -----------------------------------------------------------------------

const FString UVRProfileSubsystem::DefaultProfileName = TEXT("Default");
const int32   UVRProfileSubsystem::UserIndex          = 0;

// -----------------------------------------------------------------------
// USubsystem interface
// -----------------------------------------------------------------------

void UVRProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Always boot with a valid profile so nothing is ever null.
	// StartupProfileName is configurable in Project Settings -> VR Comfort Kit.
	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	const FString StartupName = (Settings && !Settings->StartupProfileName.IsEmpty())
		? Settings->StartupProfileName
		: DefaultProfileName;

	LoadProfile(StartupName);
}

void UVRProfileSubsystem::Deinitialize()
{
	SaveActiveProfile();
	Super::Deinitialize();
}

// -----------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------

FVRProfileData UVRProfileSubsystem::GetProjectDefaultData()
{
	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	return Settings ? Settings->DefaultProfileData : FVRProfileData();
}

FString UVRProfileSubsystem::MakeSlotName(const FString& ProfileName)
{
	// Slot format: "VRComfortKit_Profile_Default", etc.
	return FString::Printf(TEXT("VRComfortKit_Profile_%s"), *ProfileName);
}

void UVRProfileSubsystem::BroadcastDataChanged()
{
	if (ActiveProfile)
	{
		OnProfileDataChanged.Broadcast(ActiveProfile->Data);
	}
}

// -----------------------------------------------------------------------
// Profile operations
// -----------------------------------------------------------------------

void UVRProfileSubsystem::LoadProfile(const FString& ProfileName)
{
	const FString Slot = MakeSlotName(ProfileName);
	const TSubclassOf<UVRUserProfile> ResolvedClass = UVRComfortKitSettings::GetResolvedProfileClass();

	if (UGameplayStatics::DoesSaveGameExist(Slot, UserIndex))
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Slot, UserIndex);
		ActiveProfile = Cast<UVRUserProfile>(Loaded);

		if (!ActiveProfile)
		{
			// Slot exists but cast failed — corrupted or class mismatch. Make fresh.
			UE_LOG(LogTemp, Warning,
				TEXT("VRProfileSubsystem: Slot '%s' exists but could not be cast to the configured ProfileClass (%s). Creating fresh."),
				*Slot, *ResolvedClass->GetName());

			ActiveProfile = Cast<UVRUserProfile>(
				UGameplayStatics::CreateSaveGameObject(ResolvedClass));
			ActiveProfile->ProfileName = ProfileName;
			ActiveProfile->Data = GetProjectDefaultData();
		}
		else
		{
			UE_LOG(LogTemp, Log,
				TEXT("VRProfileSubsystem: Loaded profile '%s'."), *ProfileName);
		}

		OnProfileLoaded.Broadcast(ProfileName, true);
	}
	else
	{
		// No slot — create a fresh profile seeded with project default data.
		ActiveProfile = Cast<UVRUserProfile>(
			UGameplayStatics::CreateSaveGameObject(ResolvedClass));
		ActiveProfile->ProfileName = ProfileName;
		ActiveProfile->Data = GetProjectDefaultData();

		UE_LOG(LogTemp, Log,
			TEXT("VRProfileSubsystem: No saved profile '%s' found — created fresh with project defaults."), *ProfileName);

		OnProfileLoaded.Broadcast(ProfileName, false);
	}

	BroadcastDataChanged();
}

bool UVRProfileSubsystem::SaveActiveProfile()
{
	if (!ActiveProfile)
	{
		UE_LOG(LogTemp, Warning, TEXT("VRProfileSubsystem: SaveActiveProfile called with no active profile."));
		return false;
	}

	const FString Slot = MakeSlotName(ActiveProfile->ProfileName);
	const bool bSuccess = UGameplayStatics::SaveGameToSlot(ActiveProfile, Slot, UserIndex);

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error,
			TEXT("VRProfileSubsystem: Failed to save profile '%s' to slot '%s'."),
			*ActiveProfile->ProfileName, *Slot);
	}

	return bSuccess;
}

void UVRProfileSubsystem::SwitchProfile(const FString& NewProfileName)
{
	SaveActiveProfile();
	LoadProfile(NewProfileName);
}

void UVRProfileSubsystem::ResetActiveProfile(bool bSaveAfterReset)
{
	if (!ActiveProfile) return;

	ActiveProfile->ResetToDefaults(GetProjectDefaultData());

	if (bSaveAfterReset)
	{
		SaveActiveProfile();
	}

	BroadcastDataChanged();
}

bool UVRProfileSubsystem::DeleteProfile(const FString& ProfileName)
{
	if (ActiveProfile && ActiveProfile->ProfileName == ProfileName)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("VRProfileSubsystem: Cannot delete the currently active profile '%s'."), *ProfileName);
		return false;
	}

	const FString Slot = MakeSlotName(ProfileName);

	if (!UGameplayStatics::DoesSaveGameExist(Slot, UserIndex))
	{
		return false;
	}

	return UGameplayStatics::DeleteGameInSlot(Slot, UserIndex);
}

// -----------------------------------------------------------------------
// Data accessors
// -----------------------------------------------------------------------

FVRProfileData UVRProfileSubsystem::GetActiveData() const
{
	if (!ActiveProfile)
	{
		UE_LOG(LogTemp, Warning, TEXT("VRProfileSubsystem: GetActiveData called with no active profile — returning defaults."));
		return FVRProfileData();
	}
	return ActiveProfile->Data;
}

void UVRProfileSubsystem::SetActiveData(const FVRProfileData& NewData)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data = NewData;
	BroadcastDataChanged();
}

FString UVRProfileSubsystem::GetActiveProfileName() const
{
	return ActiveProfile ? ActiveProfile->ProfileName : TEXT("None");
}

// -----------------------------------------------------------------------
// Per-field setters
// -----------------------------------------------------------------------

void UVRProfileSubsystem::SetMovementMode(EVRMovementMode Mode)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.MovementMode = Mode;
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetTurnMode(EVRTurnMode Mode)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.TurnMode = Mode;
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetMoveSpeed(float Speed)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.MoveSpeed = FMath::Max(0.f, Speed);
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetSnapTurnAngle(float Degrees)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.SnapTurnAngle = FMath::Clamp(Degrees, 15.f, 180.f);
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetSmoothTurnSpeed(float DegreesPerSecond)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.SmoothTurnSpeed = FMath::Max(0.f, DegreesPerSecond);
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetVignetteEnabled(bool bEnabled)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.bVignetteEnabled = bEnabled;
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetVignetteIntensity(float Intensity)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.VignetteIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetInvertHands(bool bInvert)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.bInvertHands = bInvert;
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetDirectionMode(EVRDirectionMode Mode)
{
	if (!ActiveProfile) return;
	ActiveProfile->Data.DirectionMode = Mode;
	BroadcastDataChanged();
}

void UVRProfileSubsystem::SetGhostMode(bool bEnabled)
{
	if (!ActiveProfile) return;
	// Ghost mode is Transient — never saved, just runtime state.
	ActiveProfile->Data.bGhostMode = bEnabled;
	BroadcastDataChanged();
}
