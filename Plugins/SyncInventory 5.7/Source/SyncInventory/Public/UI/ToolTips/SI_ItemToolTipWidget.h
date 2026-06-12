#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "SI_ItemToolTipWidget.generated.h"

class USI_InventoryComponent;
class UTextBlock;
class UImage;
class UWidget;
class USI_ItemDefinition;

UCLASS()
class SYNCINVENTORY_API USI_ItemToolTipWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Inventory|Tooltip")
	void InitTooltip(USI_InventoryComponent* InInventoryComponent, const FGameplayTag& InContainerId, int32 InSlotIndex);

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StackText = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RarityText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemTypeText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemLevelText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RequiredLevelText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EquippedText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BindStateText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemTagText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ContainerIdText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotIndexText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> MaxStackSizeText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StackableText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CanUseText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ConsumeOnUseText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> AmountToConsumeText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EquippableText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EquipSlotText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ApplyCooldownOnUseText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> UseEffectCountText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EquipStatCountText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EquipStatsText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> AbilityCountText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ReferenceText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SourceContainerIdText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SourceSlotIndexText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> DescriptionText = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> ItemIconImage = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> SlotBackgroundImage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip|Rarity", meta=(Categories="Item.Rarity"))
	TMap<FGameplayTag, FSlateColor> RarityTextColors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip|Rarity")
	FSlateColor DefaultRarityTextColor = FSlateColor(FLinearColor::White);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip|Equipped")
	FSlateColor EquippedTextColor = FSlateColor(FLinearColor(1.f, 0.82f, 0.12f, 1.f));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip|Equipped")
	FSlateColor UnequippedTextColor = FSlateColor(FLinearColor::White);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip|Binding")
	FSlateColor BoundTextColor = FSlateColor(FLinearColor(1.f, 0.82f, 0.12f, 1.f));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip|Binding")
	FSlateColor UnboundTextColor = FSlateColor(FLinearColor::White);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip|Rarity")
	FSlateBrush DefaultSlotBackgroundBrush;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip|Rarity", meta=(Categories="Item.Rarity"))
	TMap<FGameplayTag, FSlateBrush> RaritySlotBackgroundBrushes;

private:
	const FSlateBrush& GetBackgroundRarityBrush(const FGameplayTag& RarityTag) const;
	
	FText GetDisplayTextFromTag(const FGameplayTag& Tag) const;
	FText GetRarityDisplayText(const FGameplayTag& RarityTag) const;
	void SetOptionalText(UTextBlock* TextBlock, const FText& Text, bool bShouldShow = true) const;
	void SetOptionalDescriptionText(const FText& Text) const;
};
