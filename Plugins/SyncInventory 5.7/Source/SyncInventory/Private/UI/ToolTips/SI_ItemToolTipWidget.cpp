#include "UI/ToolTips/SI_ItemToolTipWidget.h"
#include "Components/TextBlock.h"
#include "Component/SI_InventoryComponent.h"
#include "Data/Databases/SI_ItemDatabase.h"
#include "Data/Definitions/SI_ItemDefinition.h"
#include "Replication/SI_InventoryFastArray.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "GameplayEffect.h"


void USI_ItemToolTipWidget::InitTooltip(USI_InventoryComponent* InInventoryComponent, const FGameplayTag& InContainerId,
	int32 InSlotIndex)
{
	if (!InInventoryComponent || !InContainerId.IsValid() || InSlotIndex < 0) return;
	
	const FSI_InventoryEntry* Entry = InInventoryComponent->FindEntryAtSlot(InContainerId, InSlotIndex);
	if (!Entry || !Entry->IsValidEntry()) return;

	const USI_ItemDatabase* ItemDatabase = InInventoryComponent->GetItemDatabase();
	const USI_ItemDefinition* ItemDefinition = ItemDatabase
		? ItemDatabase->GetItemDefinitionByTag(Entry->ItemTag)
		: nullptr;

	if (!ItemDefinition) return;
	
	const FGameplayTag RarityTag = ItemDefinition->GetItemRarityTag();
	
	if (RarityText)
	{
		RarityText->SetText(GetRarityDisplayText(RarityTag));

		if (const FSlateColor* RarityColor = RarityTextColors.Find(RarityTag))
		{
			RarityText->SetColorAndOpacity(*RarityColor);
		}
		else
		{
			RarityText->SetColorAndOpacity(DefaultRarityTextColor);
		}
	}

	if (ItemTypeText)
	{
		ItemTypeText->SetText(GetDisplayTextFromTag(ItemDefinition->ItemType));
	}

	SetOptionalText(ItemLevelText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipItemLevelFormat", "Item level: {0}"),
		FText::AsNumber(ItemDefinition->ItemLevel)),
		ItemDefinition->ItemLevel > 0);

	SetOptionalText(RequiredLevelText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipRequiredLevelFormat", "Req Lv: {0}"),
		FText::AsNumber(ItemDefinition->RequiredLevel)),
		ItemDefinition->RequiredLevel > 0);

	const FGameplayTag EquipmentContainerId = InInventoryComponent->GetEquipmentContainerId();
	const bool bIsEquipped = Entry->ItemInstanceData.bIsEquipped
		|| (EquipmentContainerId.IsValid()
			&& (Entry->ContainerId == EquipmentContainerId
				|| (Entry->HasValidSourceReference() && Entry->SourceContainerId == EquipmentContainerId)));

	SetOptionalText(EquippedText, bIsEquipped
		? NSLOCTEXT("SyncInventory", "TooltipEquipped", "Equipped")
		: NSLOCTEXT("SyncInventory", "TooltipUnequipped", "Unequipped"),
		ItemDefinition->bEquippable);
	if (EquippedText)
	{
		EquippedText->SetColorAndOpacity(bIsEquipped ? EquippedTextColor : UnequippedTextColor);
	}

	SetOptionalText(BindStateText, ItemDefinition->bBoundToCharacter
		? NSLOCTEXT("SyncInventory", "TooltipBound", "Bound")
		: NSLOCTEXT("SyncInventory", "TooltipUnbound", "Unbound"));
	if (BindStateText)
	{
		BindStateText->SetColorAndOpacity(ItemDefinition->bBoundToCharacter ? BoundTextColor : UnboundTextColor);
	}

	SetOptionalText(ItemTagText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipItemTagFormat", "Item Tag: {0}"),
		FText::FromString(ItemDefinition->ItemTag.ToString())),
		ItemDefinition->ItemTag.IsValid());

	SetOptionalText(ContainerIdText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipContainerIdFormat", "Container: {0}"),
		FText::FromString(Entry->ContainerId.ToString())),
		Entry->ContainerId.IsValid());

	SetOptionalText(SlotIndexText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipSlotIndexFormat", "Slot: {0}"),
		FText::AsNumber(Entry->SlotIndex)),
		Entry->SlotIndex != INDEX_NONE);

	if (ItemNameText)
	{
		ItemNameText->SetText(ItemDefinition->ItemName);
		
		if (const FSlateColor* RarityColor = RarityTextColors.Find(RarityTag))
		{
			ItemNameText->SetColorAndOpacity(*RarityColor);
		}
		else
		{
			ItemNameText->SetColorAndOpacity(DefaultRarityTextColor);
		}
	}

	if (QuantityText)
	{
		QuantityText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		QuantityText->SetText(FText::Format(
			NSLOCTEXT("SyncInventory", "TooltipQuantityFormat", "Quantity: {0}"),
			FText::AsNumber(Entry->Quantity)));
	}

	SetOptionalText(StackText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipStackFormat", "Stack: {0} / {1}"),
		FText::AsNumber(Entry->Quantity),
		FText::AsNumber(ItemDefinition->MaxStackSize)),
		ItemDefinition->bStackable);

	SetOptionalText(MaxStackSizeText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipMaxStackSizeFormat", "Max Stack: {0}"),
		FText::AsNumber(ItemDefinition->MaxStackSize)),
		ItemDefinition->bStackable);

	SetOptionalText(StackableText, ItemDefinition->bStackable
		? NSLOCTEXT("SyncInventory", "TooltipStackable", "Stackable")
		: NSLOCTEXT("SyncInventory", "TooltipNotStackable", "Not Stackable"));

	SetOptionalText(CanUseText, ItemDefinition->bCanUse
		? NSLOCTEXT("SyncInventory", "TooltipCanUse", "Usable")
		: NSLOCTEXT("SyncInventory", "TooltipCannotUse", "Not Usable"));

	SetOptionalText(ConsumeOnUseText, ItemDefinition->bConsumeOnUse
		? NSLOCTEXT("SyncInventory", "TooltipConsumeOnUse", "Consumed On Use")
		: NSLOCTEXT("SyncInventory", "TooltipNotConsumeOnUse", "Not Consumed On Use"),
		ItemDefinition->bCanUse);

	SetOptionalText(AmountToConsumeText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipAmountToConsumeFormat", "Consumes: {0}"),
		FText::AsNumber(ItemDefinition->AmountToConsume)),
		ItemDefinition->bCanUse && ItemDefinition->bConsumeOnUse);

	SetOptionalText(EquippableText, ItemDefinition->bEquippable
		? NSLOCTEXT("SyncInventory", "TooltipEquippable", "Equippable")
		: NSLOCTEXT("SyncInventory", "TooltipNotEquippable", "Not Equippable"));

	SetOptionalText(EquipSlotText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipEquipSlotFormat", "Equip Slot: {0}"),
		GetDisplayTextFromTag(ItemDefinition->EquipSlot)),
		ItemDefinition->bEquippable && ItemDefinition->EquipSlot.IsValid());

	const UGameplayEffect* CooldownEffect = ItemDefinition->CooldownGameplayEffect
		? ItemDefinition->CooldownGameplayEffect.GetDefaultObject()
		: nullptr;
	SetOptionalText(CooldownText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipCooldownFormat", "Cooldown: {0}"),
		CooldownEffect ? FText::FromString(CooldownEffect->GetClass()->GetName()) : FText::GetEmpty()),
		ItemDefinition->bCanUse && CooldownEffect != nullptr);

	SetOptionalText(ApplyCooldownOnUseText, ItemDefinition->bApplyCooldownOnUse
		? NSLOCTEXT("SyncInventory", "TooltipApplyCooldownOnUse", "Applies Cooldown On Use")
		: NSLOCTEXT("SyncInventory", "TooltipDoesNotApplyCooldownOnUse", "Does Not Apply Cooldown On Use"),
		ItemDefinition->bCanUse && CooldownEffect != nullptr);

	SetOptionalText(UseEffectCountText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipUseEffectCountFormat", "Use Effects: {0}"),
		FText::AsNumber(ItemDefinition->UseGameplayEffects.Num())),
		ItemDefinition->UseGameplayEffects.Num() > 0);

	SetOptionalText(EquipStatCountText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipEquipStatCountFormat", "Equip Stats: {0}"),
		FText::AsNumber(ItemDefinition->EquipStats.Num())),
		ItemDefinition->EquipStats.Num() > 0);

	FString EquipStatsString;
	for (const FSI_ItemStatModifier& EquipStat : ItemDefinition->EquipStats)
	{
		if (!EquipStat.StatTag.IsValid()) continue;

		const FString StatName = GetDisplayTextFromTag(EquipStat.StatTag).ToString();
		const FString MagnitudeText = FMath::IsNearlyEqual(EquipStat.Magnitude, FMath::RoundToFloat(EquipStat.Magnitude))
			? FString::FromInt(FMath::RoundToInt(EquipStat.Magnitude))
			: FString::SanitizeFloat(EquipStat.Magnitude);

		FString OperationPrefix;
		switch (EquipStat.Operation.GetValue())
		{
		case EGameplayModOp::Additive:
			OperationPrefix = EquipStat.Magnitude >= 0.f ? TEXT("+") : TEXT("");
			break;
		case EGameplayModOp::Multiplicitive:
			OperationPrefix = TEXT("x");
			break;
		case EGameplayModOp::Division:
			OperationPrefix = TEXT("/");
			break;
		case EGameplayModOp::Override:
			OperationPrefix = TEXT("=");
			break;
		default:
			break;
		}

		if (!EquipStatsString.IsEmpty())
		{
			EquipStatsString += LINE_TERMINATOR;
		}

		EquipStatsString += FString::Printf(TEXT("%s%s %s"), *OperationPrefix, *MagnitudeText, *StatName);
	}

	SetOptionalText(EquipStatsText, FText::FromString(EquipStatsString), ItemDefinition->EquipStats.Num() > 0);

	SetOptionalText(AbilityCountText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipAbilityCountFormat", "Abilities: {0}"),
		FText::AsNumber(ItemDefinition->GameplayAbilities.Num())),
		ItemDefinition->GameplayAbilities.Num() > 0);

	SetOptionalText(ReferenceText, Entry->bIsReference
		? NSLOCTEXT("SyncInventory", "TooltipReferenceSlot", "Reference Slot")
		: NSLOCTEXT("SyncInventory", "TooltipRealSlot", "Inventory Slot"));

	SetOptionalText(SourceContainerIdText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipSourceContainerIdFormat", "Source Container: {0}"),
		FText::FromString(Entry->SourceContainerId.ToString())),
		Entry->HasValidSourceReference());

	SetOptionalText(SourceSlotIndexText, FText::Format(
		NSLOCTEXT("SyncInventory", "TooltipSourceSlotIndexFormat", "Source Slot: {0}"),
		FText::AsNumber(Entry->SourceSlotIndex)),
		Entry->HasValidSourceReference());
	
	if (ItemIconImage)
	{
		ItemIconImage->SetVisibility(ItemDefinition->ItemIcon
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);

		if (ItemDefinition->ItemIcon)
		{
			ItemIconImage->SetBrushFromTexture(ItemDefinition->ItemIcon);
		}
	}
	
	if (SlotBackgroundImage)
	{
		SlotBackgroundImage->SetBrush(GetBackgroundRarityBrush(RarityTag));
	}

	SetOptionalDescriptionText(ItemDefinition->Description);
}

const FSlateBrush& USI_ItemToolTipWidget::GetBackgroundRarityBrush(const FGameplayTag& RarityTag) const
{
	if (const FSlateBrush* ExactBrush = RaritySlotBackgroundBrushes.Find(RarityTag))
	{
		return *ExactBrush;
	}

	for (const TPair<FGameplayTag, FSlateBrush>& Pair : RaritySlotBackgroundBrushes)
	{
		if (Pair.Key.IsValid() && RarityTag.MatchesTag(Pair.Key))
		{
			return Pair.Value;
		}
	}

	return DefaultSlotBackgroundBrush;
}

FText USI_ItemToolTipWidget::GetDisplayTextFromTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid())
	{
		return FText::GetEmpty();
	}

	FString TagString = Tag.ToString();

	FString RightSide;
	if (TagString.Split(TEXT("."), nullptr, &RightSide, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		TagString = RightSide;
	}

	return FText::FromString(TagString);
}

FText USI_ItemToolTipWidget::GetRarityDisplayText(const FGameplayTag& RarityTag) const
{
	return GetDisplayTextFromTag(RarityTag);
}

void USI_ItemToolTipWidget::SetOptionalText(UTextBlock* TextBlock, const FText& Text, bool bShouldShow) const
{
	if (!TextBlock)
	{
		return;
	}

	const bool bHasText = !Text.IsEmpty();
	TextBlock->SetVisibility(bShouldShow && bHasText
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);
	TextBlock->SetText(bShouldShow ? Text : FText::GetEmpty());
}

void USI_ItemToolTipWidget::SetOptionalDescriptionText(const FText& Text) const
{
	if (!DescriptionText)
	{
		return;
	}

	UTextBlock* DescriptionTextBlock = Cast<UTextBlock>(DescriptionText);
	if (!DescriptionTextBlock)
	{
		DescriptionText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetOptionalText(DescriptionTextBlock, Text);
}
