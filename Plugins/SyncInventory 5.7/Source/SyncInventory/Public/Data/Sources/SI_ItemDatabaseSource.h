#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/Databases/SI_ItemDefinitionCategory.h"
#include "SI_ItemDatabaseSource.generated.h"

class USI_ItemDefinition;

UCLASS(BlueprintType)
class SYNCINVENTORY_API USI_ItemDatabaseSource : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Database", meta = (TitleProperty = "CategoryName"))
	TArray<FSI_ItemDefinitionCategory> Categories;

	UFUNCTION(BlueprintCallable, Category = "Database")
	const USI_ItemDefinition* GetItemDefinitionByTag(const FGameplayTag& ItemTag) const;

protected:
	virtual void AppendDefinitions(TArray<const USI_ItemDefinition*>& OutDefinitions) const;
};
