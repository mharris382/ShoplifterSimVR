// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ShoplifterAIDataAsset.generated.h"

USTRUCT(BlueprintType)
struct SHOPLIFTERSIMULATOR_API FAIJobDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jobs", meta = (Categories = "AI.Job"))
	FGameplayTag JobID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jobs")
	int JobPriority = 0;
};





UCLASS()
class SHOPLIFTERSIMULATOR_API UShoplifterAIDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags", meta = (Categories = "AI.Traits"))
	FGameplayTagContainer StaticTraits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags", meta=(Categories ="AI.Factions"))
	FGameplayTag FactionID;

	//This is here for future implementation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags", meta = (Categories = "AI.Job"))
	TArray<FAIJobDefinition> Jobs;




	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float BaseWalkingSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MaxMoveSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Endurance")
	float BaseEndurance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Endurance")
	float EnduranceRecoveryRate = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Endurance")
	float EnduranceConsumptionRate = 5.f;



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Senses")
	float BaselineSightRange = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Senses")
	float BaselineHearingRange = 500.f;
};
