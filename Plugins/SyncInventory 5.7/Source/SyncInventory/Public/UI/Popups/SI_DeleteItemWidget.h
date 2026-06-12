#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/SI_MovableResizableWidgetBase.h"
#include "SI_DeleteItemWidget.generated.h"

class USI_InventoryComponent;
class USI_InventoryUIManager;
class UTextBlock;
class UButton;
class USizeBox;
class UWidget;

UCLASS()
class SYNCINVENTORY_API USI_DeleteItemWidget : public USI_MovableResizableWidgetBase
{
	GENERATED_BODY()
	
public:
	void InitDeleteItemWidget(USI_InventoryUIManager* InInventoryUIManager, USI_InventoryComponent* InInventoryComponent,
		const FGameplayTag& InContainerId, int32 InSlotIndex);
	void SetPopScreenPosition(const FVector2D& ScreenPosition);
	void RequestClose();
	
protected:
	virtual void NativeConstruct() override;
	virtual UWidget* GetManipulationTarget() const override;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> DragHandle = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<USizeBox> PopupRoot = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> ConfirmDeleteButton = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> CancelDeleteButton = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText = nullptr;
	
	UPROPERTY()
	TWeakObjectPtr<USI_InventoryUIManager> InventoryUIManager;
	
	UPROPERTY()
	TWeakObjectPtr<USI_InventoryComponent> InventoryComponent;
	
	UPROPERTY()
	FGameplayTag ContainerId;
	
	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;
	
private:
	void Refresh();
	
	UFUNCTION()
	void HandleConfirmDeleteClicked();

	UFUNCTION()
	void HandleCancelDeleteClicked();
};
