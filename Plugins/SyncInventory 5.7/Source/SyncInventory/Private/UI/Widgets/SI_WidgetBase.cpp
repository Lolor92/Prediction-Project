#include "UI/Widgets/SI_WidgetBase.h"
#include "Component/SI_InventoryUIManager.h"
#include "UI/DragDrop/SI_InventoryDragOperation.h"
#include "UI/Widgets/SI_InventoryGridWidget.h"
#include "Components/Widget.h"

void USI_WidgetBase::InitWidget(USI_InventoryUIManager* InInventoryUIManager, const FGameplayTag& InContainerId, const int32 InMaxSlots,
                                int32 InColumns)
{
	InventoryUIManager = InInventoryUIManager;
	ContainerId = InContainerId;
	MaxSlots = InMaxSlots;
	Columns = InColumns;
	
	if (GridWidget)
	{
		GridWidget->InitGridWidget(InInventoryUIManager, InContainerId, InMaxSlots, InColumns);
	}
}

void USI_WidgetBase::RefreshSlots(const TArray<int32>& SlotIndices)
{
	if (GridWidget)
	{
		GridWidget->RefreshSlots(SlotIndices);
	}
}

void USI_WidgetBase::PressedFeedback(int32 SlotIndex)
{
	if (GridWidget)
	{
		GridWidget->PressedFeedback(SlotIndex);
	}
}

void USI_WidgetBase::ResetPressedFeedback(int32 SlotIndex)
{
	if (GridWidget)
	{
		GridWidget->ResetPressedFeedback(SlotIndex);
	}
}

bool USI_WidgetBase::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                  UDragDropOperation* InOperation)
{
	if (!InOperation) return false;
	
	USI_InventoryDragOperation* DragOperation = Cast<USI_InventoryDragOperation>(InOperation);
	if (!DragOperation) return false;
	
	// Dropping on the window background still counts as dropping inside the inventory UI.
	// Only a true drag cancel (release outside valid inventory UI) should open the delete prompt.
	return true;
}

UWidget* USI_WidgetBase::GetManipulationTarget() const
{
	if (WindowRoot)
	{
		return WindowRoot.Get();
	}

	return Cast<UWidget>(GridWidget);
}
