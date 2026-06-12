#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/Sources/SI_ItemDatabaseSource.h"
#include "SI_ItemDatabase.generated.h"


UCLASS(BlueprintType)
class SYNCINVENTORY_API USI_ItemDatabase : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Database|Sources")
	TArray<TObjectPtr<USI_ItemDatabaseSource>> Databases;
	
	UFUNCTION(BlueprintCallable, Category="Database")
	const USI_ItemDefinition* GetItemDefinitionByTag(const FGameplayTag& ItemTag) const;
};
