#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/SI_MovableResizableWidgetBase.h"
#include "SI_WidgetBase.generated.h"


class USI_InventoryGridWidget;
class USI_InventoryUIManager;
class UWidget;

UCLASS()
class SYNCINVENTORY_API USI_WidgetBase : public USI_MovableResizableWidgetBase
{
	GENERATED_BODY()
	
public:
	virtual void InitWidget(USI_InventoryUIManager* InInventoryUIManager, const FGameplayTag& InContainerId, const int32 InMaxSlots, int32 InColumns);
	virtual void RefreshSlots(const TArray<int32>& SlotIndices);
	
	void PressedFeedback(int32 SlotIndex);
	void ResetPressedFeedback(int32 SlotIndex);
	
protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual UWidget* GetManipulationTarget() const override;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory|UI")
	TObjectPtr<UWidget> WindowRoot = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|UI")
	TObjectPtr<USI_InventoryGridWidget> GridWidget = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_InventoryUIManager> InventoryUIManager;
	
	UPROPERTY(Transient)
	FGameplayTag ContainerId;
	
	UPROPERTY(Transient)
	int32 MaxSlots;
	
	UPROPERTY(Transient)
	int32 Columns;
};
