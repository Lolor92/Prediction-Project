#include "UI/Notifications/SI_ItemAddedNotificationWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void USI_ItemAddedNotificationWidget::InitNotification(const FSI_ItemAddedNotificationData& InNotificationData)
{
	NotificationData = InNotificationData;

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetRenderOpacity(1.f);

	ApplyNotificationData();
	PlayIntroAnimation();
}

void USI_ItemAddedNotificationWidget::ApplyNotificationData()
{
	const FSlateColor* RarityTextColor = RarityTextColors.Find(NotificationData.ItemRarityTag);
	const FSlateColor& TextColor = RarityTextColor ? *RarityTextColor : DefaultTextColor;
	const bool bUseMessageText = NotificationData.bUseMessageText && !NotificationData.MessageText.IsEmpty();
	const bool bHasDedicatedMessageTextBlock = MessageTextBlock != nullptr;

	if (ItemIconImage)
	{
		if (!bUseMessageText && NotificationData.ItemIcon)
		{
			ItemIconImage->SetBrushFromTexture(NotificationData.ItemIcon);
			ItemIconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ItemIconImage->SetBrushFromTexture(nullptr);
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (ItemNameText)
	{
		const FText DisplayText = bUseMessageText
			? (bHasDedicatedMessageTextBlock ? FText::GetEmpty() : NotificationData.MessageText)
			: NotificationData.ItemName;

		ItemNameText->SetText(DisplayText);
		ItemNameText->SetColorAndOpacity(TextColor);
		ItemNameText->SetVisibility(bUseMessageText && bHasDedicatedMessageTextBlock
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);
	}

	if (MessageTextBlock)
	{
		MessageTextBlock->SetText(bUseMessageText ? NotificationData.MessageText : FText::GetEmpty());
		MessageTextBlock->SetColorAndOpacity(TextColor);
		MessageTextBlock->SetVisibility(bUseMessageText
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (QuantityText)
	{
		if (bUseMessageText)
		{
			QuantityText->SetText(FText::GetEmpty());
			QuantityText->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		const FText QuantityDisplayText = FText::Format(
			NSLOCTEXT("SyncInventory", "ItemAddedNotificationQuantity", "x{0}"),
			FText::AsNumber(FMath::Max(0, NotificationData.QuantityAdded))
		);

		QuantityText->SetText(QuantityDisplayText);
		QuantityText->SetColorAndOpacity(TextColor);
		QuantityText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}
