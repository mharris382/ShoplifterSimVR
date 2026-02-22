// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ShoplifterVRPawn.h"

// Sets default values
AShoplifterVRPawn::AShoplifterVRPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AShoplifterVRPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AShoplifterVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AShoplifterVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

