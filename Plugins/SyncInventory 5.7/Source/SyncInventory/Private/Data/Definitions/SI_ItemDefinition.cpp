#include "Data/Definitions/SI_ItemDefinition.h"
#include "Tags/SI_NativeGameplayTags.h"

USI_ItemDefinition::USI_ItemDefinition()
{
	ItemRarityTag = TAG_SI_Item_Rarity_Common;
}

FGameplayTag USI_ItemDefinition::GetItemRarityTag() const
{
	if (ItemRarityTag.IsValid())
	{
		return ItemRarityTag;
	}

	switch (ItemRarity)
	{
	case ESI_ItemRarity::Common:
		return TAG_SI_Item_Rarity_Common;
	case ESI_ItemRarity::Rare:
		return TAG_SI_Item_Rarity_Rare;
	case ESI_ItemRarity::Epic:
		return TAG_SI_Item_Rarity_Epic;
	case ESI_ItemRarity::Legendary:
		return TAG_SI_Item_Rarity_Legendary;
	default:
		return TAG_SI_Item_Rarity_Common;
	}
}

#if WITH_EDITOR
#include "GameplayEffect.h"
#include "Misc/DataValidation.h"

EDataValidationResult USI_ItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (!ItemTag.IsValid())
	{
		Context.AddError(NSLOCTEXT("Inventory", "MissingItemTag", "ItemTag is not assigned."));
		Result = EDataValidationResult::Invalid;
	}

	if (!ItemType.IsValid())
	{
		Context.AddError(NSLOCTEXT("Inventory", "MissingItemType", "ItemType is not assigned."));
		Result = EDataValidationResult::Invalid;
	}

	const FGameplayTag EffectiveRarityTag = GetItemRarityTag();
	if (!EffectiveRarityTag.IsValid())
	{
		Context.AddError(NSLOCTEXT("Inventory", "MissingItemRarity", "ItemRarity is not assigned."));
		Result = EDataValidationResult::Invalid;
	}

	if (bEquippable)
	{
		if (!EquipSlot.IsValid())
		{
			Context.AddError(NSLOCTEXT("Inventory", "MissingEquipSlot", "Equippable items must have an EquipSlot."));
			Result = EDataValidationResult::Invalid;
		}

	}

	if (CooldownGameplayEffect)
	{
		const UGameplayEffect* CooldownEffectCDO = CooldownGameplayEffect.GetDefaultObject();
		if (!CooldownEffectCDO)
		{
			Context.AddError(NSLOCTEXT("Inventory", "InvalidCooldownGameplayEffect", "CooldownGameplayEffect is invalid."));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif
