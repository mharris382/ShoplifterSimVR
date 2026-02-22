// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Controllers/ShoplifterAIControllerBase.h"
#include "GameFramework/Character.h"
#include "AI/Pawns/ShoplifterAIBase.h"



bool AShoplifterAIControllerBase::IsValidAI() const
{
	if (CachedShoplifterPawn && CachedShoplifterPawn->IsValidAI())
		return true;
	return false;
}

void AShoplifterAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if(AShoplifterAIBase* ShoplifterPawn = Cast<AShoplifterAIBase>(InPawn))
	{
		CachedShoplifterPawn = ShoplifterPawn;
	}
}

void AShoplifterAIControllerBase::OnUnPossess()
{
	Super::OnUnPossess();
	CachedShoplifterPawn = nullptr;
}

void AShoplifterAIControllerBase::ActorsPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	Super::ActorsPerceptionUpdated(UpdatedActors);
}

