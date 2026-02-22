// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "ShoplifterAIControllerBase.generated.h"

class AShoplifterAIBase;

/**
 * 
 */
UCLASS()
class SHOPLIFTERSIMULATOR_API AShoplifterAIControllerBase : public AAIController
{
	GENERATED_BODY()
	


public:
	bool IsValidAI() const;


	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void ActorsPerceptionUpdated(const TArray< AActor* >& UpdatedActors) override;



private:
	TObjectPtr<AShoplifterAIBase> CachedShoplifterPawn = nullptr;

};
