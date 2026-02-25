#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRMenuAnchor.h"
#include "VRMenuContextComponent.generated.h"

class UCameraComponent;
class UMotionControllerComponent;
class UWidgetInteractionComponent;
class UUserWidget;
class UInputAction;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVRMenuOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVRMenuClosed);

/**
 * UVRMenuContextComponent
 *
 * Attach to your VR Pawn. Owns the full lifecycle of opening and closing
 * a world-space VR menu — including camera fade, player snap, widget
 * interaction laser, IMC swap, and clean restoration on close.
 *
 * The game decides WHEN to open the menu (call OpenMenu from anywhere).
 * The menu decides when to close itself (call CloseMenu on this component),
 * or it can be closed externally. The close input action in project settings
 * fires CloseMenu automatically while the menu is open.
 *
 * Usage:
 *   // Open with default widget class from project settings, no anchor
 *   MenuContext->OpenMenu();
 *
 *   // Open a specific widget at a placed anchor
 *   MenuContext->OpenMenu(UMyInventoryWidget::StaticClass(), MyAnchorActor);
 *
 *   // Close from anywhere
 *   MenuContext->CloseMenu();
 */
UCLASS(ClassGroup = "VRComfortKit", meta = (BlueprintSpawnableComponent))
class VRCOMFORTKIT_API UVRMenuContextComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UVRMenuContextComponent();

	// -----------------------------------------------------------------------
	// Initialization — call from pawn BeginPlay
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Menu")
	void InitializeComponents(
		UCameraComponent* InCamera,
		UMotionControllerComponent* InDominantController);

	// -----------------------------------------------------------------------
	// Public API
	// -----------------------------------------------------------------------

	/**
	 * Opens the VR menu.
	 *
	 * @param WidgetClass  The UUserWidget subclass to spawn. If null, falls
	 *                     back to DefaultMenuWidgetClass in project settings.
	 * @param Anchor       Optional world anchor. If null, widget spawns
	 *                     relative to the HMD at MenuSpawnDistance.
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Menu",
		meta = (AdvancedDisplay = "Anchor"))
	void OpenMenu(
		UPARAM(meta = (AllowAbstract = false)) TSubclassOf<UUserWidget> WidgetClass = nullptr,
		AVRMenuAnchor* Anchor = nullptr);

	/**
	 * Closes the active menu and restores gameplay state.
	 * Safe to call even if no menu is open (no-ops cleanly).
	 */
	UFUNCTION(BlueprintCallable, Category = "VRComfortKit|Menu")
	void CloseMenu();

	UFUNCTION(BlueprintPure, Category = "VRComfortKit|Menu")
	bool IsMenuOpen() const { return bMenuOpen; }

	/** Returns the currently active menu widget, or null if no menu is open. */
	UFUNCTION(BlueprintPure, Category = "VRComfortKit|Menu")
	UUserWidget* GetActiveWidget() const { return ActiveWidget; }

	// -----------------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "VRComfortKit|Menu")
	FOnVRMenuOpened OnMenuOpened;

	UPROPERTY(BlueprintAssignable, Category = "VRComfortKit|Menu")
	FOnVRMenuClosed OnMenuClosed;

	// -----------------------------------------------------------------------
	// UActorComponent interface
	// -----------------------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

	// -----------------------------------------------------------------------
	// Fade helpers
	// -----------------------------------------------------------------------

	void FadeOut(FSimpleDelegate OnComplete);
	void FadeIn();

	UFUNCTION()
	void OnOpenFadeOutComplete();

	UFUNCTION()
	void OnCloseFadeOutComplete();

	// -----------------------------------------------------------------------
	// Open / close steps
	// -----------------------------------------------------------------------

	void CommitOpen();
	void CommitClose();

	void SnapPlayerToAnchor();
	void RestorePlayerTransform();

	void SpawnWidget();
	void DespawnWidget();

	void ActivateLaser();
	void DeactivateLaser();

	void PushMenuIMC();
	void PopMenuIMC();

	void BindCloseAction();
	void UnbindCloseAction();

	UFUNCTION()
	void OnCloseActionTriggered();

	// -----------------------------------------------------------------------
	// Cached component refs
	// -----------------------------------------------------------------------

	UPROPERTY()
	UCameraComponent* Camera = nullptr;

	UPROPERTY()
	UMotionControllerComponent* DominantController = nullptr;

	UPROPERTY()
	UWidgetInteractionComponent* WidgetInteraction = nullptr;

	// -----------------------------------------------------------------------
	// Runtime state
	// -----------------------------------------------------------------------

	bool bMenuOpen = false;

	UPROPERTY()
	UUserWidget* ActiveWidget = nullptr;

	/** The actor wrapping the widget in world space. */
	UPROPERTY()
	AActor* WidgetActor = nullptr;

	/** Anchor used for this open session — may be null. */
	UPROPERTY()
	AVRMenuAnchor* ActiveAnchor = nullptr;

	/** Player transform captured just before snapping, for restore on close. */
	FTransform PreMenuPlayerTransform;

	/** Widget class resolved at OpenMenu time. */
	TSubclassOf<UUserWidget> PendingWidgetClass;

	/** Whether we should restore the player transform on close. */
	bool bShouldRestoreTransform = false;

	/** Fade timer handle for open/close sequencing. */
	FTimerHandle FadeTimerHandle;
};
