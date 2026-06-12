#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Templates/SubclassOf.h"
#include "TimerManager.h"
#include "Types/Structs/SI_CooldownTypes.h"
#include "SI_InventorySlotWidget.generated.h"


class USI_InventorySlotDragVisual;
class USI_InventoryUIManager;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTextBlock;
class UUserWidget;

UCLASS()
class SYNCINVENTORY_API USI_InventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitSlotWidget(USI_InventoryUIManager* InInventoryUIManager, const FGameplayTag& InContainerId ,int32 InSlotIndex);
	
	void Refresh();
	void SetToDefault();
	
	void PressedFeedback();
	void ResetPressedFeedback();
	
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	int32 GetSlotIndex() const { return SlotIndex; }
	
	const FSlateBrush& GetBackgroundRarityBrush(const FGameplayTag& RarityTag) const;
	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Drag")
	TSubclassOf<USI_InventorySlotDragVisual> DragVisualClass;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_InventoryUIManager> InventoryUIManager = nullptr;
	
	UPROPERTY(Transient)
	FGameplayTag ContainerId;
	
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> SlotBackgroundImage = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText = nullptr;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InputTagText = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> HoverImage = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> DeactivatedImageOverlay = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> CooldownOverlayImage = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Cooldown")
	TObjectPtr<UMaterialInterface> CooldownMaterial = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Cooldown")
	FName CooldownProgressParameterName = TEXT("Progress");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush DefaultSlotBackgroundBrush;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity", meta=(Categories="Item.Rarity"))
	TMap<FGameplayTag, FSlateBrush> RaritySlotBackgroundBrushes;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush CommonSlotBackgroundBrush;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush RareSlotBackgroundBrush;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush EpicSlotBackgroundBrush;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush LegendarySlotBackgroundBrush;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI")
	float PressedScale = 0.94f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Tooltip")
	TSubclassOf<UUserWidget> ItemTooltipWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Tooltip")
	FVector2D TooltipPadding = FVector2D(5.f, 5.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Tooltip")
	FVector2D TooltipOffset = FVector2D(0.f, 0.f);
	
private:
	void RefreshCooldownVisual();
	void StartCooldownRefreshTimer();
	void StopCooldownRefreshTimer();
	void ApplyCooldownVisual(const FSI_SlotCooldownState& CooldownState);
	
	void ShowItemTooltip(const FGeometry& InGeometry);
	void HideItemTooltip();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveItemTooltipWidget = nullptr;
	
	UPROPERTY(Transient)
	FTimerHandle CooldownRefreshTimer;
	
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownMaterialInstance = nullptr;
	
	UPROPERTY(Transient)
	bool bDragDetectThisClick = false;
};
