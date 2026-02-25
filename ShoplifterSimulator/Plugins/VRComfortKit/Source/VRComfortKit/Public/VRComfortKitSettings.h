#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Blueprint/UserWidget.h"
#include "VRUserProfile.h"
#include "VRComfortKitSettings.generated.h"

/**
 * UVRComfortKitSettings
 *
 * Project-wide configuration for the VR Comfort Kit plugin.
 * Accessible via Project Settings → Plugins → VR Comfort Kit.
 *
 * The most important setting is ProfileClass — subclass UVRUserProfile
 * to add your own persisted per-profile data, then point this at your
 * subclass. The subsystem will instantiate and serialize your class
 * automatically.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "VR Comfort Kit"))
class VRCOMFORTKIT_API UVRComfortKitSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UVRComfortKitSettings();

	// -----------------------------------------------------------------------
	// Profile class
	// -----------------------------------------------------------------------

	/**
	 * The save game class used for VR comfort profiles.
	 * Override this with a subclass of UVRUserProfile to add
	 * additional persisted data (e.g. custom accessibility settings,
	 * game-specific preferences) without modifying plugin code.
	 *
	 * Changing this at runtime has no effect — it is read once on
	 * subsystem initialization.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Profile",
		meta = (DisplayName = "Profile Save Game Class"))
	TSubclassOf<UVRUserProfile> ProfileClass;

	// -----------------------------------------------------------------------
	// Default profile data
	// -----------------------------------------------------------------------

	/**
	 * The data applied to any newly created profile slot (including Default).
	 * These are the out-of-box values for your project — override them here
	 * rather than hunting for magic numbers in code.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Profile|Defaults",
		meta = (DisplayName = "Default Profile Data"))
	FVRProfileData DefaultProfileData;

	/**
	 * Name of the profile slot loaded on subsystem startup.
	 * Typically "Default". Change this if you manage profile selection
	 * yourself before the subsystem initializes.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Profile",
		meta = (DisplayName = "Startup Profile Name"))
	FString StartupProfileName = TEXT("Default");

	// -----------------------------------------------------------------------
	// UDeveloperSettings interface
	// -----------------------------------------------------------------------

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	// -----------------------------------------------------------------------
	// Input
	// -----------------------------------------------------------------------

	/** Mapping context added at high priority while a menu is open. Should contain UI navigation bindings. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> MenuMappingContext;

	/** Input action that closes the active menu. Bound automatically when a menu opens, unbound on close. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> IA_CloseMenu;

	// -----------------------------------------------------------------------
	// Menu
	// -----------------------------------------------------------------------

	/**
	 * Fallback widget class used when OpenMenu() is called with no class argument.
	 * Set this to your main settings/pause menu widget.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Menu",
		meta = (AllowAbstract = false))
	TSubclassOf<UUserWidget> DefaultMenuWidgetClass;

	/** Whether to fade the camera to FadeColor during menu open/close transitions. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Menu|Fade")
	bool bFadeOnMenuTransition = true;

	/** Duration in seconds of each fade (out and in). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Menu|Fade",
		meta = (EditCondition = "bFadeOnMenuTransition", ClampMin = "0.05", ClampMax = "2.0"))
	float FadeDuration = 0.25f;

	/** Color the screen fades to during transitions. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Menu|Fade",
		meta = (EditCondition = "bFadeOnMenuTransition"))
	FLinearColor FadeColor = FLinearColor::Black;

	/**
	 * Distance in cm from the HMD that the menu widget spawns when no AVRMenuAnchor is provided.
	 * 150cm is a comfortable default reading distance.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Menu",
		meta = (ClampMin = "50.0", ClampMax = "500.0"))
	float MenuSpawnDistance = 150.f;

	/** Returns the CDO. Safe to call from any thread after engine init. */
	static const UVRComfortKitSettings* Get()
	{
		return GetDefault<UVRComfortKitSettings>();
	}

	/**
	 * Returns the resolved profile class — falls back to UVRUserProfile
	 * if ProfileClass is unset (e.g. fresh project with no config).
	 */
	UFUNCTION(BlueprintPure, Category = "VRComfortKit|Settings")
	static TSubclassOf<UVRUserProfile> GetResolvedProfileClass();
};
