#include "VRMenuAnchor.h"
#include "Components/SceneComponent.h"

#if WITH_EDITOR
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#endif

AVRMenuAnchor::AVRMenuAnchor()
{
	PrimaryActorTick.bCanEverTick = false;

	AnchorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AnchorRoot"));
	SetRootComponent(AnchorRoot);

	PlayerSnapPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerSnapPoint"));
	PlayerSnapPoint->SetupAttachment(AnchorRoot);
	PlayerSnapPoint->SetRelativeLocation(FVector::ZeroVector);

	WidgetSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WidgetSpawnPoint"));
	WidgetSpawnPoint->SetupAttachment(AnchorRoot);
	// Sensible default: 150cm in front, 120cm up — comfortable reading distance
	WidgetSpawnPoint->SetRelativeLocation(FVector(150.f, 0.f, 120.f));
	// Face back toward the player snap point
	WidgetSpawnPoint->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));

#if WITH_EDITORONLY_DATA
	BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	if (BillboardComponent)
	{
		BillboardComponent->SetupAttachment(AnchorRoot);
		BillboardComponent->bIsScreenSizeScaled = true;
	}

	PlayerFacingArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("PlayerFacingArrow"));
	if (PlayerFacingArrow)
	{
		PlayerFacingArrow->SetupAttachment(PlayerSnapPoint);
		PlayerFacingArrow->ArrowColor = FColor::Green;
		PlayerFacingArrow->bTreatAsASprite = true;
		PlayerFacingArrow->SetRelativeScale3D(FVector(2.f));
	}

	WidgetFacingArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("WidgetFacingArrow"));
	if (WidgetFacingArrow)
	{
		WidgetFacingArrow->SetupAttachment(WidgetSpawnPoint);
		WidgetFacingArrow->ArrowColor = FColor::Blue;
		WidgetFacingArrow->bTreatAsASprite = true;
	}
#endif
}

void AVRMenuAnchor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Keep the widget facing arrow aligned to the widget spawn point's forward
	// so designers get accurate visual feedback in the editor viewport.
#if WITH_EDITORONLY_DATA
	if (WidgetFacingArrow)
	{
		WidgetFacingArrow->SetWorldRotation(WidgetSpawnPoint->GetComponentRotation());
	}
#endif
}

FTransform AVRMenuAnchor::GetPlayerSnapTransform() const
{
	return PlayerSnapPoint->GetComponentTransform();
}

FTransform AVRMenuAnchor::GetWidgetSpawnTransform() const
{
	return WidgetSpawnPoint->GetComponentTransform();
}
