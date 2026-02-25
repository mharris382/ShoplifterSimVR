#include "VRMenuContextComponent.h"
#include "VRComfortKitSettings.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "TimerManager.h"

UVRMenuContextComponent::UVRMenuContextComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// -----------------------------------------------------------------------
// UActorComponent interface
// -----------------------------------------------------------------------

void UVRMenuContextComponent::BeginPlay()
{
	Super::BeginPlay();

	// Create the widget interaction component — attached to dominant controller,
	// disabled until a menu opens.
	WidgetInteraction = NewObject<UWidgetInteractionComponent>(GetOwner(), TEXT("VRWidgetInteraction"));
	WidgetInteraction->RegisterComponent();
	WidgetInteraction->InteractionDistance = 1000.f;
	WidgetInteraction->InteractionSource = EWidgetInteractionSource::Custom;
	WidgetInteraction->bShowDebug = false;
	WidgetInteraction->Deactivate();
}

void UVRMenuContextComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Force-close without animation if we're ending play mid-menu
	if (bMenuOpen)
	{
		GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
		CommitClose();
	}

	Super::EndPlay(EndPlayReason);
}

// -----------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------

void UVRMenuContextComponent::InitializeComponents(
	UCameraComponent* InCamera,
	UMotionControllerComponent* InDominantController)
{
	Camera = InCamera;
	DominantController = InDominantController;

	if (WidgetInteraction && DominantController)
	{
		WidgetInteraction->AttachToComponent(
			DominantController,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void UVRMenuContextComponent::OpenMenu(TSubclassOf<UUserWidget> WidgetClass, AVRMenuAnchor* Anchor)
{
	if (bMenuOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("VRMenuContextComponent: OpenMenu called while menu already open — ignored."));
		return;
	}

	// Resolve widget class — prefer argument, fall back to project settings
	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	PendingWidgetClass = WidgetClass
		? WidgetClass
		: (Settings ? Settings->DefaultMenuWidgetClass : nullptr);

	if (!PendingWidgetClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("VRMenuContextComponent: OpenMenu called with no widget class and no DefaultMenuWidgetClass set in project settings."));
		return;
	}

	ActiveAnchor = Anchor;
	bMenuOpen = true;

	const bool bFade = Settings && Settings->bFadeOnMenuTransition;

	if (bFade)
	{
		FadeOut(FSimpleDelegate::CreateUObject(this, &UVRMenuContextComponent::OnOpenFadeOutComplete));
	}
	else
	{
		CommitOpen();
	}
}

void UVRMenuContextComponent::CloseMenu()
{
	if (!bMenuOpen) return;

	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	const bool bFade = Settings && Settings->bFadeOnMenuTransition;

	// Unbind close action immediately so it can't fire twice during the fade
	UnbindCloseAction();
	PopMenuIMC();

	if (bFade)
	{
		FadeOut(FSimpleDelegate::CreateUObject(this, &UVRMenuContextComponent::OnCloseFadeOutComplete));
	}
	else
	{
		CommitClose();
	}
}

// -----------------------------------------------------------------------
// Fade
// -----------------------------------------------------------------------

void UVRMenuContextComponent::FadeOut(FSimpleDelegate OnComplete)
{
	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	const float Duration  = Settings ? Settings->FadeDuration  : 0.25f;
	const FLinearColor Color = Settings ? Settings->FadeColor : FLinearColor::Black;

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;

	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, Duration, Color, false, true);
	}

	// Fire the completion delegate after fade duration
	FTimerDelegate Delegate;
	Delegate.BindLambda([OnComplete]() { OnComplete.ExecuteIfBound(); });
	GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, Delegate, Duration, false);
}

void UVRMenuContextComponent::FadeIn()
{
	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	const float Duration  = Settings ? Settings->FadeDuration  : 0.25f;
	const FLinearColor Color = Settings ? Settings->FadeColor : FLinearColor::Black;

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;

	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, Duration, Color, false, false);
	}
}

void UVRMenuContextComponent::OnOpenFadeOutComplete()
{
	CommitOpen();
	FadeIn();
}

void UVRMenuContextComponent::OnCloseFadeOutComplete()
{
	CommitClose();
	FadeIn();
}

// -----------------------------------------------------------------------
// Open / close steps
// -----------------------------------------------------------------------

void UVRMenuContextComponent::CommitOpen()
{
	PreMenuPlayerTransform = GetOwner()->GetActorTransform();

	SnapPlayerToAnchor();
	SpawnWidget();
	ActivateLaser();
	PushMenuIMC();
	BindCloseAction();

	OnMenuOpened.Broadcast();
}

void UVRMenuContextComponent::CommitClose()
{
	DespawnWidget();
	DeactivateLaser();
	RestorePlayerTransform();

	bMenuOpen = false;
	ActiveAnchor = nullptr;

	OnMenuClosed.Broadcast();
}

// -----------------------------------------------------------------------
// Player snap
// -----------------------------------------------------------------------

void UVRMenuContextComponent::SnapPlayerToAnchor()
{
	if (!ActiveAnchor)
	{
		// No anchor — player stays put, widget will spawn relative to HMD in SpawnWidget()
		bShouldRestoreTransform = false;
		return;
	}

	bShouldRestoreTransform = ActiveAnchor->bRestoreTransformOnClose;

	FTransform SnapTransform = ActiveAnchor->GetPlayerSnapTransform();

	if (!ActiveAnchor->bSnapPlayerRotation)
	{
		// Preserve current yaw, only move position
		SnapTransform.SetRotation(GetOwner()->GetActorQuat());
	}

	GetOwner()->SetActorTransform(SnapTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void UVRMenuContextComponent::RestorePlayerTransform()
{
	if (!bShouldRestoreTransform) return;

	GetOwner()->SetActorTransform(
		PreMenuPlayerTransform,
		false, nullptr,
		ETeleportType::TeleportPhysics);
}

// -----------------------------------------------------------------------
// Widget spawn
// -----------------------------------------------------------------------

void UVRMenuContextComponent::SpawnWidget()
{
	if (!PendingWidgetClass || !GetWorld()) return;

	// Determine spawn transform
	FTransform SpawnTransform;

	if (ActiveAnchor)
	{
		SpawnTransform = ActiveAnchor->GetWidgetSpawnTransform();
	}
	else if (Camera)
	{
		// HMD-relative fallback: project forward from camera, flatten to eye level
		const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
		const float Distance = Settings ? Settings->MenuSpawnDistance : 150.f;

		FVector Forward = Camera->GetForwardVector();
		Forward.Z = 0.f;
		if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
		Forward.Normalize();

		const FVector Location = Camera->GetComponentLocation() + Forward * Distance;

		// Widget faces the player
		const FRotator Rotation = (-Forward).Rotation();

		SpawnTransform = FTransform(Rotation, Location);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("VRMenuContextComponent: No anchor and no camera — spawning widget at origin."));
		SpawnTransform = FTransform::Identity;
	}

	// Spawn an actor with a WidgetComponent to host the widget in world space
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	WidgetActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), SpawnTransform, Params);

	if (!WidgetActor) return;

	UWidgetComponent* WidgetComp = NewObject<UWidgetComponent>(WidgetActor, TEXT("MenuWidget"));
	WidgetComp->RegisterComponent();
	WidgetComp->SetWidgetSpace(EWidgetSpace::World);
	WidgetComp->SetWidgetClass(PendingWidgetClass);
	WidgetComp->SetDrawSize(FVector2D(800.f, 600.f));
	WidgetComp->SetWorldTransform(SpawnTransform);
	WidgetActor->SetRootComponent(WidgetComp);

	WidgetComp->InitWidget();
	ActiveWidget = WidgetComp->GetUserWidgetObject();
}

void UVRMenuContextComponent::DespawnWidget()
{
	ActiveWidget = nullptr;

	if (WidgetActor)
	{
		WidgetActor->Destroy();
		WidgetActor = nullptr;
	}
}

// -----------------------------------------------------------------------
// Laser pointer
// -----------------------------------------------------------------------

void UVRMenuContextComponent::ActivateLaser()
{
	if (!WidgetInteraction) return;

	WidgetInteraction->Activate();

	// Point the interaction component along the controller's forward vector
	if (DominantController)
	{
		WidgetInteraction->SetWorldTransform(DominantController->GetComponentTransform());
	}
}

void UVRMenuContextComponent::DeactivateLaser()
{
	if (WidgetInteraction)
	{
		WidgetInteraction->Deactivate();
	}
}

// -----------------------------------------------------------------------
// IMC management
// -----------------------------------------------------------------------

void UVRMenuContextComponent::PushMenuIMC()
{
	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	if (!Settings) return;

	UInputMappingContext* MenuIMC = Settings->MenuMappingContext.LoadSynchronous();
	if (!MenuIMC) return;

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	if (!PC) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());

	if (Subsystem)
	{
		// High priority so menu IMC shadows locomotion bindings
		Subsystem->AddMappingContext(MenuIMC, 10);
	}
}

void UVRMenuContextComponent::PopMenuIMC()
{
	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	if (!Settings) return;

	UInputMappingContext* MenuIMC = Settings->MenuMappingContext.LoadSynchronous();
	if (!MenuIMC) return;

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	if (!PC) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());

	if (Subsystem)
	{
		Subsystem->RemoveMappingContext(MenuIMC);
	}
}

// -----------------------------------------------------------------------
// Close action binding
// -----------------------------------------------------------------------

void UVRMenuContextComponent::BindCloseAction()
{
	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	if (!Settings) return;

	UInputAction* CloseAction = Settings->IA_CloseMenu.LoadSynchronous();
	if (!CloseAction) return;

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return;

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
	if (!EIC) return;

	EIC->BindAction(CloseAction, ETriggerEvent::Triggered, this,
		&UVRMenuContextComponent::OnCloseActionTriggered);
}

void UVRMenuContextComponent::UnbindCloseAction()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return;

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
	if (!EIC) return;

	const UVRComfortKitSettings* Settings = UVRComfortKitSettings::Get();
	if (!Settings) return;

	UInputAction* CloseAction = Settings->IA_CloseMenu.LoadSynchronous();
	if (!CloseAction) return;

	EIC->ClearBindingsForObject(this);
}

void UVRMenuContextComponent::OnCloseActionTriggered()
{
	CloseMenu();
}
