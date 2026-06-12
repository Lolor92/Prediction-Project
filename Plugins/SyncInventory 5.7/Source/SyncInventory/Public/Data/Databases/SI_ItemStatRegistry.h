#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SI_ItemStatRegistry.generated.h"

USTRUCT(BlueprintType)
struct FSI_ItemStatAttributeMapping
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Stats", meta = (Categories = "Stat"))
	FGameplayTag StatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Stats")
	FGameplayAttribute Attribute;
};

UCLASS(BlueprintType)
class SYNCINVENTORY_API USI_ItemStatRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	bool FindAttributeForStatTag(const FGameplayTag& StatTag, FGameplayAttribute& OutAttribute) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Stats", meta = (TitleProperty = "StatTag"))
	TArray<FSI_ItemStatAttributeMapping> AttributeMappings;
};
