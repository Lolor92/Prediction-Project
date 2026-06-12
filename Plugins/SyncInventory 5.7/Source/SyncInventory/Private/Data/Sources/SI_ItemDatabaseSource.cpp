#include "Data/Sources/SI_ItemDatabaseSource.h"

const USI_ItemDefinition* USI_ItemDatabaseSource::GetItemDefinitionByTag(const FGameplayTag& ItemTag) const
{
	if (!ItemTag.IsValid())
	{
		return nullptr;
	}

	TArray<const USI_ItemDefinition*> Definitions;
	AppendDefinitions(Definitions);

	for (const USI_ItemDefinition* Definition : Definitions)
	{
		if (IsValid(Definition) && Definition->ItemTag == ItemTag)
		{
			return Definition;
		}
	}

	return nullptr;
}

void USI_ItemDatabaseSource::AppendDefinitions(TArray<const USI_ItemDefinition*>& OutDefinitions) const
{
	for (const FSI_ItemDefinitionCategory& Category : Categories)
	{
		for (const USI_ItemDefinition* Definition : Category.Definitions)
		{
			if (IsValid(Definition))
			{
				OutDefinitions.Add(Definition);
			}
		}
	}
}
