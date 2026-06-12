#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/SI_MovableResizableWidgetBase.h"
#include "SI_SplitStackWidget.generated.h"

class USI_InventoryComponent;
class USI_InventoryUIManager;
class USizeBox;
class USlider;
class UButton;
class UTextBlock;
class UWidget;

UCLASS(Blueprintable, BlueprintType)
class SYNCINVENTORY_API USI_SplitStackWidget : public USI_MovableResizableWidgetBase
{
	GENERATED_BODY()
	
public:
	void InitSplitStackWidget(USI_InventoryUIManager* InInventoryUIManager, USI_InventoryComponent* InInventoryComponent,
		const FGameplayTag& InContainerId, int32 InSlotIndex, int32 InCurrentQuantity);
	
	void SetPopupScreenPosition(const FVector2D& ScreenPosition);
	void RequestClose();
	
protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual UWidget* GetManipulationTarget() const override;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> DragHandle = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<USizeBox> PopupRoot = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<USlider> RemainingQuantitySlider = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> ConfirmSplitButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RemainingQuantityText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SplitQuantityText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentQuantityText = nullptr;
	
private:
	void RefreshDisplay();
	
	UFUNCTION()
	void HandleRemainingQuantitySliderChanged(float Value);
	
	UFUNCTION()
	void HandleConfirmSplitClicked();
	
	int32 GetRemainingQuantityFromSlider() const;
	int32 GetSplitQuantity() const;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_InventoryComponent> InventoryComponent;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_InventoryUIManager> InventoryUIManager;
	
	UPROPERTY(Transient)
	FGameplayTag ContainerId;
	
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;
	
	UPROPERTY(Transient)
	int32 CurrentQuantity = 0;
	
	UPROPERTY(Transient)
	int32 MinRemainingQuantity = 1;
	
	UPROPERTY(Transient)
	int32 MaxRemainingQuantity = 1;
};
