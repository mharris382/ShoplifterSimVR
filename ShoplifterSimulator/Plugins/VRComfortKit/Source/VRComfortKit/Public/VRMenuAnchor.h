#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRMenuAnchor.generated.h"

class UBillboardComponent;
class UArrowComponent;

/**
 * AVRMenuAnchor
 *
 * Optional placeable actor that defines where the player snaps to when
 * a VR menu opens, and where the menu widget spawns in the world.
 *
 * If no anchor is provided to UVRMenuContextComponent::OpenMenu(), the
 * context falls back to spawning the widget relative to the HMD.
 *
 * PlayerSnapTransform  — where the player pawn root is moved to on open.
 *                        Face this toward the widget spawn point.
 * WidgetSpawnPoint     — where the world-space widget actor spawns.
 *                        Typically in front of and slightly below eye level
 *                        from the snap point.
 */
UCLASS(BlueprintType, Blueprintable, HideCategories = (Cooking, Replication, Input))
class VRCOMFORTKIT_API AVRMenuAnchor : public AActor
{
	GENERATED_BODY()

public:

	AVRMenuAnchor();

	// -----------------------------------------------------------------------
	// Components
	// -----------------------------------------------------------------------

	/** Root — defines the anchor's world position. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRMenuAnchor")
	TObjectPtr<USceneComponent> AnchorRoot;

	/**
	 * Where the player pawn is teleported on menu open.
	 * The pawn's root will be placed here; forward vector should face
	 * toward the WidgetSpawnPoint.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRMenuAnchor")
	TObjectPtr<USceneComponent> PlayerSnapPoint;

	/**
	 * Where the menu widget actor spawns.
	 * Position and rotation are passed directly to the widget actor —
	 * set this to a comfortable reading distance and height.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRMenuAnchor")
	TObjectPtr<USceneComponent> WidgetSpawnPoint;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UArrowComponent> PlayerFacingArrow;

	UPROPERTY()
	TObjectPtr<UArrowComponent> WidgetFacingArrow;

	UPROPERTY()
	TObjectPtr<UBillboardComponent> BillboardComponent;
#endif

	// -----------------------------------------------------------------------
	// Configuration
	// -----------------------------------------------------------------------

	/**
	 * If true, the player's yaw is overridden to match PlayerSnapPoint's
	 * forward direction on open. If false, only position is snapped and
	 * the player keeps their current facing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRMenuAnchor|Snap")
	bool bSnapPlayerRotation = true;

	/**
	 * If true, restores the player to their pre-menu transform when the
	 * menu closes. If false, the player stays at the snap point.
	 * Useful for menus that serve as "room" transitions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRMenuAnchor|Snap")
	bool bRestoreTransformOnClose = true;

	// -----------------------------------------------------------------------
	// Accessors
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "VRMenuAnchor")
	FTransform GetPlayerSnapTransform() const;

	UFUNCTION(BlueprintPure, Category = "VRMenuAnchor")
	FTransform GetWidgetSpawnTransform() const;

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
};
