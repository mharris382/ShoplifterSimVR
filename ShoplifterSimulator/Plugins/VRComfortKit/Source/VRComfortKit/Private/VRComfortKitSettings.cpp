#include "VRComfortKitSettings.h"

UVRComfortKitSettings::UVRComfortKitSettings()
{
	// Default the class to the base plugin type so the setting is never null
	// in a fresh project — devs only need to change it if they subclass.
	ProfileClass = UVRUserProfile::StaticClass();
}

TSubclassOf<UVRUserProfile> UVRComfortKitSettings::GetResolvedProfileClass()
{
	const UVRComfortKitSettings* Settings = Get();
	if (Settings && Settings->ProfileClass)
	{
		return Settings->ProfileClass;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("VRComfortKitSettings: ProfileClass is null — falling back to UVRUserProfile. "
		     "Check Project Settings → Plugins → VR Comfort Kit."));

	return UVRUserProfile::StaticClass();
}
