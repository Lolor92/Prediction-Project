// Copyright ProjectLogos

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "SyncCombatAbilitySet.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

USTRUCT()
struct FSyncCombatAbilitySetGrantedHandles
{
	GENERATED_BODY()

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void TakeFromAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent);

private:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
};

USTRUCT(BlueprintType)
struct FSyncCombatAbilityGrant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="Ability")
	TSubclassOf<UGameplayAbility> AbilityClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Ability", meta=(ClampMin="1"))
	int32 Level = 1;
};

UCLASS(BlueprintType)
class SYNCCOMBAT_API USyncCombatAbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Abilities", meta=(TitleProperty="AbilityClass"))
	TArray<FSyncCombatAbilityGrant> Abilities;

	void GiveToAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent,
		FSyncCombatAbilitySetGrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;
};
