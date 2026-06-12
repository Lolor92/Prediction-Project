#include "UI/DragDrop/SI_InventorySlotDragVisual.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Component/SI_InventoryComponent.h"
#include "Data/Definitions/SI_ItemDefinition.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tags/SI_NativeGameplayTags.h"

void USI_InventorySlotDragVisual::InitDragVisual(USI_InventoryComponent* InInventoryComponent,
	const FGameplayTag& InContainerId, const int32 InSlotIndex)
{
	InventoryComponent = InInventoryComponent;
	ContainerId = InContainerId;
	SlotIndex = InSlotIndex;
	
	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	
	if (!Entry || !ContainerId.IsValid() || SlotIndex < 0)
	{
		SetToDefault();
		return;
	}
	
	FGameplayTag ItemTag = Entry->ItemTag;
	if (!ItemTag.IsValid())
	{
		SetToDefault();
		return;
	}
	
	const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
	const USI_ItemDefinition* Def = ItemDatabase ? ItemDatabase->GetItemDefinitionByTag(ItemTag) : nullptr;
	if (!Def)
	{
		SetToDefault();
		return;
	}
	
	if (SlotBackgroundImage)
	{
		SlotBackgroundImage->SetBrush(GetBackgroundRarityBrush(Def->GetItemRarityTag()));
	}
	
	if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ItemIcon->SetBrushFromTexture(Def->ItemIcon);
	}
	
	if (QuantityText)
	{
		// References show the amount carried in the main inventory.
		// Real inventory slots show their own stack quantity.
		const int32 DisplayQuantity = Entry->bIsReference
			? InventoryComponent->GetMainInventoryQuantityForItem(Entry->ItemTag)
			: Entry->Quantity;

		const bool bShowQuantity = Def->bStackable && DisplayQuantity > 0;

		QuantityText->SetVisibility(bShowQuantity
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

		QuantityText->SetText(bShowQuantity
			? FText::AsNumber(DisplayQuantity) : FText::GetEmpty());
	}
	
	if (CooldownText)
	{
		CooldownText->SetVisibility(ESlateVisibility::Hidden);
		CooldownText->SetText(FText::GetEmpty());
	}
	
	if (CooldownOverlayImage)
	{
		CooldownOverlayImage->SetVisibility(ESlateVisibility::Hidden);
	}
	
	FSI_SlotCooldownState CooldownState{};
	
	if (!InventoryComponent || !ContainerId.IsValid() || SlotIndex < 0) return;
	
	UAbilitySystemComponent* ASC = InventoryComponent->GetAbilitySystemComponent();
	if (!ASC) return;
	if (!Def->CooldownGameplayEffect) return;

	FGameplayEffectQuery Query;
	Query.EffectDefinition = Def->CooldownGameplayEffect;

	const TArray<TPair<float, float>> TimePairs = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);
	if (TimePairs.IsEmpty()) return;
	
	for (const TPair<float, float>& TimePair : TimePairs)
	{
		const float Remaining = FMath::Max(0.f, TimePair.Key);
		const float Duration = FMath::Max(0.f, TimePair.Value);
		
		if (Remaining > CooldownState.TimeRemaining)
		{
			CooldownState.TimeRemaining = Remaining;
			CooldownState.TotalDuration = Duration;
		}
	}
	
	CooldownState.bIsOnCooldown = CooldownState.TimeRemaining > 0.f;
	
	ApplyCooldownVisual(CooldownState);
}

void USI_InventorySlotDragVisual::SetToDefault()
{
	if (SlotBackgroundImage)
	{
		SlotBackgroundImage->SetBrush(DefaultSlotBackgroundBrush);
	}
	
	if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		ItemIcon->SetBrushFromTexture(nullptr);
	}
	
	if (QuantityText)
	{
		QuantityText->SetVisibility(ESlateVisibility::Hidden);
		QuantityText->SetText(FText::GetEmpty());
	}
	
	if (CooldownText)
	{
		CooldownText->SetVisibility(ESlateVisibility::Hidden);
		CooldownText->SetText(FText::GetEmpty());
	}
	
	if (CooldownOverlayImage)
	{
		CooldownOverlayImage->SetVisibility(ESlateVisibility::Hidden);
	}
	
	ApplyCooldownVisual(FSI_SlotCooldownState{});
}

const FSlateBrush& USI_InventorySlotDragVisual::GetBackgroundRarityBrush(const FGameplayTag& RarityTag) const
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

	if (RarityTag.MatchesTagExact(TAG_SI_Item_Rarity_Common))
	{
		return CommonSlotBackgroundBrush;
	}

	if (RarityTag.MatchesTagExact(TAG_SI_Item_Rarity_Rare))
	{
		return RareSlotBackgroundBrush;
	}

	if (RarityTag.MatchesTagExact(TAG_SI_Item_Rarity_Epic))
	{
		return EpicSlotBackgroundBrush;
	}

	if (RarityTag.MatchesTagExact(TAG_SI_Item_Rarity_Legendary))
	{
		return LegendarySlotBackgroundBrush;
	}

	return DefaultSlotBackgroundBrush;
}

void USI_InventorySlotDragVisual::ApplyCooldownVisual(const FSI_SlotCooldownState& CooldownState)
{
	const bool bShowCooldown = CooldownState.bIsOnCooldown;
	
	if (CooldownText)
	{
		CooldownText->SetText(bShowCooldown
			? FText::AsNumber(FMath::Max(1, FMath::CeilToInt(CooldownState.TimeRemaining)))
			: FText::GetEmpty());
		CooldownText->SetVisibility(bShowCooldown ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	if (DeactivatedImageOverlay)
	{
		DeactivatedImageOverlay->SetVisibility(bShowCooldown ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	if (!CooldownOverlayImage) return;

	if (bShowCooldown && !CooldownMaterialInstance && CooldownMaterial)
	{
		CooldownMaterialInstance = UMaterialInstanceDynamic::Create(CooldownMaterial, this);
		if (CooldownMaterialInstance)
		{
			CooldownOverlayImage->SetBrushFromMaterial(CooldownMaterialInstance.Get());
		}
	}

	if (!bShowCooldown || !CooldownMaterialInstance)
	{
		CooldownOverlayImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	CooldownMaterialInstance->SetScalarParameterValue(
		CooldownProgressParameterName,
		CooldownState.GetNormalizedRemaining());

	CooldownOverlayImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}
