#include "Data/Databases/SI_ItemStatRegistry.h"

bool USI_ItemStatRegistry::FindAttributeForStatTag(const FGameplayTag& StatTag, FGameplayAttribute& OutAttribute) const
{
	if (!StatTag.IsValid()) return false;

	for (const FSI_ItemStatAttributeMapping& Mapping : AttributeMappings)
	{
		if (!Mapping.StatTag.IsValid()) continue;
		if (!Mapping.Attribute.IsValid()) continue;

		if (Mapping.StatTag == StatTag)
		{
			OutAttribute = Mapping.Attribute;
			return true;
		}
	}

	return false;
}
