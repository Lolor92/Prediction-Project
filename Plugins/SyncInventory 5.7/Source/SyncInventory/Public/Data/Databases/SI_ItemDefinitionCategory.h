#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/SI_ItemDefinition.h"
#include "SI_ItemDefinitionCategory.generated.h"

USTRUCT(BlueprintType)
struct FSI_ItemDefinitionCategory
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Database")
	FName CategoryName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Database")
	TArray<TObjectPtr<USI_ItemDefinition>> Definitions;
};
