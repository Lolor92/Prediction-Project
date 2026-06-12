#include "Data/Databases/SI_ItemDatabase.h"

const USI_ItemDefinition* USI_ItemDatabase::GetItemDefinitionByTag(const FGameplayTag& ItemTag) const
{
	if (!ItemTag.IsValid()) return nullptr;

	for (const USI_ItemDatabaseSource* Database : Databases)
	{
		if (!IsValid(Database))
		{
			continue;
		}

		if (const USI_ItemDefinition* ItemDefinition = Database->GetItemDefinitionByTag(ItemTag))
		{
			return ItemDefinition;
		}
	}

	return nullptr;
}
