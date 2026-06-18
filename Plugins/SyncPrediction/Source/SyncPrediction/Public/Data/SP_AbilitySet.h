#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpec.h"
#include "SP_AbilitySet.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct FSP_AbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sync Prediction|Abilities")
	TSubclassOf<UGameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sync Prediction|Abilities")
	int32 AbilityLevel = 1;
};

USTRUCT(BlueprintType)
struct FSP_AbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void TakeFromAbilitySystem(UAbilitySystemComponent* ASC);

private:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
};

UCLASS(BlueprintType, Const)
class SYNCPREDICTION_API USP_AbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	void GiveToAbilitySystem(UAbilitySystemComponent* ASC, FSP_AbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sync Prediction|Abilities", meta=(TitleProperty="Ability"))
	TArray<FSP_AbilitySet_GameplayAbility> GrantedGameplayAbilities;
};
