#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "SI_ItemAddedNotificationWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

USTRUCT(BlueprintType)
struct FSI_ItemAddedNotificationData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Notification", meta=(Categories="Item.Id"))
	FGameplayTag ItemTag;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Notification")
	FText ItemName;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Notification")
	TObjectPtr<UTexture2D> ItemIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Notification", meta=(Categories="Item.Rarity"))
	FGameplayTag ItemRarityTag;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Notification")
	int32 QuantityAdded = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Notification")
	FText MessageText;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Notification")
	bool bUseMessageText = false;
};

UCLASS()
class SYNCINVENTORY_API USI_ItemAddedNotificationWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Notification")
	void InitNotification(const FSI_ItemAddedNotificationData& InNotificationData);

	UFUNCTION(BlueprintPure, Category = "Inventory|Notification")
	FSI_ItemAddedNotificationData GetNotificationData() const { return NotificationData; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Notification")
	void PlayIntroAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Notification")
	void PlayPushAnimation(int32 StackIndex, FVector2D TargetTranslation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Notification")
	void PlayExitAnimation();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory|Notification")
	TObjectPtr<UImage> ItemIconImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory|Notification")
	TObjectPtr<UTextBlock> ItemNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory|Notification")
	TObjectPtr<UTextBlock> MessageTextBlock = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory|Notification")
	TObjectPtr<UTextBlock> QuantityText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Notification")
	FSlateColor DefaultTextColor = FSlateColor(FLinearColor::White);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Notification", meta=(Categories="Item.Rarity"))
	TMap<FGameplayTag, FSlateColor> RarityTextColors;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Inventory|Notification")
	FSI_ItemAddedNotificationData NotificationData;
	
private:
	void ApplyNotificationData();
};
