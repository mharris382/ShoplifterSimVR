// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Pawns/ShoplifterAIBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Logging/LogMacros.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY(LogShoplifterAI);

#pragma region LIFE_CYCLE

// Sets default values
AShoplifterAIBase::AShoplifterAIBase()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called to bind functionality to input
void AShoplifterAIBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


// Called when the game starts or when spawned
void AShoplifterAIBase::BeginPlay()
{
	Super::BeginPlay();
	CachedMovementComponent = GetCharacterMovement();
	check(CachedMovementComponent);
	if(IsValidAI())
	{
		OnDataAssetChanged_Internal();
	}
	else
	{
		UE_LOG(LogShoplifterAI, Error, TEXT("AShoplifterAIBase %s has no valid AIDataAsset assigned!"), *GetName());
	}
}


void AShoplifterAIBase::OnDataAssetChanged_Internal()
{
	check(AIDataAsset);
	CachedMovementComponent = GetCharacterMovement();
	check(CachedMovementComponent)
	UE_LOG(LogShoplifterAI, Verbose, TEXT("AShoplifterAIBase %s data asset changed to %s"), *GetName(), *AIDataAsset->GetName());
	
	InitializeCachedTags();
	OnDataAssetChanged();

}


// Called every frame
void AShoplifterAIBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


#pragma endregion



#pragma region TAGS


FGameplayTag AShoplifterAIBase::GetFaction() const
{
	if (!IsValidAI())
		return FGameplayTag();
	return AIDataAsset->FactionID;
}

FGameplayTagContainer AShoplifterAIBase::GetAllEffectiveTags() const
{
	auto tags = FGameplayTagContainer(GetFaction());
	if (!IsValidAI())
		return tags;
	tags.AppendTags(AIDataAsset->StaticTraits);
	tags.AppendTags(CurrentState.DynamicTags);
	return tags;
}
void AShoplifterAIBase::InitializeCachedTags()
{
	if (!IsValidAI())
		return;

	CachedStaticTags = FGameplayTagContainer(GetFaction());
	CachedStaticTags.AppendTags(AIDataAsset->StaticTraits);
}
bool AShoplifterAIBase::HasTag(const FGameplayTag& TagToCheck) const
{
	return GetAllEffectiveTags().HasTag(TagToCheck);
}

bool AShoplifterAIBase::HasStateTag(const FGameplayTag& TagToCheck) const
{
	return CurrentState.DynamicTags.HasTag(TagToCheck);
}

bool AShoplifterAIBase::TryAddStateTag(const FGameplayTag& TagToAdd)
{
	if (CurrentState.DynamicTags.HasTag(TagToAdd))
		return true;
	FString msg;
	if (!CanAddStateTag(TagToAdd, msg))
	{
		UE_LOG(LogShoplifterAI, Warning, TEXT("AShoplifterAIBase %s cannot add tag %s to state tags: %s"), *GetName(), *TagToAdd.ToString(), *msg);
		return false;
	}
	CurrentState.DynamicTags.AddTag(TagToAdd);
	OnDynamicTagAdded(TagToAdd);
	OnDynamicTagsChanged();
	return true;
}

bool AShoplifterAIBase::CanAddStateTag(const FGameplayTag& TagToAdd, FString& FailureReason) const
{
	if (TagToAdd.IsValid() == false)
	{
		FailureReason = FString::Printf(TEXT("Tag %s is not valid!"), *TagToAdd.ToString());
		return false;
	}

	return true;
}

bool AShoplifterAIBase::TryRemoveStateTag(const FGameplayTag& TagToRemove)
{
	if (!CurrentState.DynamicTags.HasTag(TagToRemove))
		return false;

	FString msg;
	if (!CanRemoveStateTag(TagToRemove, msg))
	{
		UE_LOG(LogShoplifterAI, Warning, TEXT("AShoplifterAIBase %s cannot remove tag %s from state tags: %s"), *GetName(), *TagToRemove.ToString(), *msg);
		return false;
	}
	
	CurrentState.DynamicTags.RemoveTag(TagToRemove);
	OnDynamicTagRemoved(TagToRemove);
	OnDynamicTagsChanged();

	return true;
}

bool AShoplifterAIBase::CanRemoveStateTag(const FGameplayTag& TagToRemove, FString& FailureReason) const
{
	if (TagToRemove.IsValid() == false)
	{
		FailureReason = FString::Printf(TEXT("Tag %s is not valid!"), *TagToRemove.ToString());
		return false;
	}
	return true;
}





#pragma endregion


// =========== Callbacks =================

