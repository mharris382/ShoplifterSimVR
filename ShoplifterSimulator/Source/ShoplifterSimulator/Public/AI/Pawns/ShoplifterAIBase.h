// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AI/Data/ShoplifterAIDataAsset.h"
//#include "GameplayTagContainer.h"
#include "GameplayTagContainer.h"
#include "ShoplifterAIBase.generated.h"

// Declares a log category symbol other .cpp files can reference
DECLARE_LOG_CATEGORY_EXTERN(LogShoplifterAI, Log, All);

class UCharacterMovementComponent;
class AActor;

USTRUCT()
struct SHOPLIFTERSIMULATOR_API FShoplifterAIDesiredState
{
	GENERATED_BODY()

	//range [-1,1] this is a value between min/max speed that determines how fast the AI moves as well as how much endurance is consumed by movement
	float NormalizedMovementSpeed = 0.f;
};

USTRUCT()
struct SHOPLIFTERSIMULATOR_API FShoplifterAIState
{
	GENERATED_BODY()


	
	float Endurance = 0.f;
	float MovementSpeed = 0.f;

	

	float Alertness = 0.f; 

	float SightEffectiveness = 1.0f;
	float HearingEffectiveness = 1.0f;


	//may have multiple targets selected (which the AI is thinking about), but only one "focused" target (which the AI is currently acting on and synced to the AI BB)
	TObjectPtr<AActor> FocusedTarget = nullptr; 
	TArray<TObjectPtr<AActor>> SelectedTargets;


	FGameplayTagContainer DynamicTags;
};

USTRUCT()
struct SHOPLIFTERSIMULATOR_API FShoplifterAIWitnessedEvent
{
	GENERATED_BODY()
	
	FName EventName;
	float EventTime = 0.f;
	FTransform EventLocation;

	FGameplayTagContainer EventDescription;
	FGameplayTagContainer PerpetratorDescription;
	
};


UCLASS()
class SHOPLIFTERSIMULATOR_API AShoplifterAIBase : public ACharacter
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UShoplifterAIDataAsset> AIDataAsset;

	// Sets default values for this character's properties
	AShoplifterAIBase();

	bool IsValidAI() const { return AIDataAsset != nullptr; }
	

	FGameplayTag GetFaction() const;
	FGameplayTagContainer GetAllEffectiveTags() const;

	bool HasTag(const FGameplayTag& TagToCheck) const;
	bool HasStateTag(const FGameplayTag& TagToCheck) const;

	bool TryAddStateTag(const FGameplayTag& TagToAdd);
	virtual bool CanAddStateTag(const FGameplayTag& TagToAdd, FString& FailureReason) const;

	bool TryRemoveStateTag(const FGameplayTag& TagToRemove);
	virtual bool CanRemoveStateTag(const FGameplayTag& TagToAdd, FString& FailureReason) const;

	bool HasJobs() const { return false; }//will implement later



private:

	void InitializeCachedTags();
	void OnDataAssetChanged_Internal();

	FGameplayTagContainer CachedStaticTags;
	FShoplifterAIState CurrentState;
	TObjectPtr<UCharacterMovementComponent> CachedMovementComponent;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnDataAssetChanged() {};

	virtual void OnDynamicTagsChanged() {};
	virtual void OnDynamicTagAdded(FGameplayTag AddedTag) {};
	virtual void OnDynamicTagRemoved(FGameplayTag AddedTag) {};


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
