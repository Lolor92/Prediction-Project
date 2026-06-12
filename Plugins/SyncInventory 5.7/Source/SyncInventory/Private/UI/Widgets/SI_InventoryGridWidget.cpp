#include "UI/Widgets/SI_InventoryGridWidget.h"
#include "Components/UniformGridPanel.h"
#include "Component/SI_InventoryComponent.h"
#include "Component/SI_InventoryUIManager.h"
#include "Data/Databases/SI_SlotLayoutConfig.h"
#include "UI/Widgets/SI_InventorySlotWidget.h"

void USI_InventoryGridWidget::InitGridWidget(USI_InventoryUIManager* InInventoryUIManager, const FGameplayTag& InContainerId,
		const int32 InMaxSlots, int32 InColumns)
{
	InventoryUIManager = InInventoryUIManager;
	ContainerId = InContainerId;
	MaxSlots = InMaxSlots;
	Columns = InColumns;
	
	BuildGrid();
}

void USI_InventoryGridWidget::BuildGrid()
{
	if (!SlotGridPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("Inventory Grid Widget could not find SlotGrid"));
		return;
	}
	
	if (!SlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Inventory Grid Widget has no SlotWidgetClass set"));
		return;
	}
	
	SlotGridPanel->ClearChildren();
	
	SlotWidgets.Reset();
	SlotWidgets.SetNum(MaxSlots);
	
	// Start with no layout config.
	// If the container does not have one, the grid will use normal row/column placement.
	const USI_SlotLayoutConfig* LayoutConfig = nullptr;

	// The UI manager owns access to the inventory component.
	if (InventoryUIManager)
	{
		const USI_InventoryComponent* InventoryComponent = InventoryUIManager->GetInventoryComponent();

		if (InventoryComponent)
		{
			LayoutConfig = InventoryComponent->GetSlotLayoutConfig(ContainerId);
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; SlotIndex++)
	{
		int32 Row = SlotIndex / Columns;
		int32 Column = SlotIndex % Columns;

		if (LayoutConfig)
		{
			// Find custom layout data for this slot index.
			const FSI_SlotLayoutEntry* SlotEntry = LayoutConfig->FindSlotEntryByIndex(SlotIndex);

			// Only use custom placement when this specific slot says to override the grid.
			if (SlotEntry && SlotEntry->bOverrideGridPosition)
			{
				Row = SlotEntry->GridRow;
				Column = SlotEntry->GridColumn;
			}
		}

		// Create one slot widget for this slot index.
		USI_InventorySlotWidget* SlotWidget = CreateWidget<USI_InventorySlotWidget>(GetOwningPlayer(), SlotWidgetClass);

		// If widget creation failed, skip this slot.
		if (!SlotWidget) continue;

		
		SlotWidget->InitSlotWidget(InventoryUIManager, ContainerId, SlotIndex);
		SlotGridPanel->AddChildToUniformGrid(SlotWidget, Row, Column);
		SlotWidgets[SlotIndex] = SlotWidget;
	}
}

void USI_InventoryGridWidget::RefreshSlots(const TArray<int32>& SlotIndices)
{
	for (int32 SlotIndex : SlotIndices)
	{
		if (!SlotWidgets.IsValidIndex(SlotIndex)) continue;
		
		SlotWidgets[SlotIndex]->Refresh();
	}
}

void USI_InventoryGridWidget::PressedFeedback(int32 SlotIndex)
{
	if (!SlotWidgets.IsValidIndex(SlotIndex))
	{
		return;
	}

	if (USI_InventorySlotWidget* SlotWidget = SlotWidgets[SlotIndex])
	{
		SlotWidget->PressedFeedback();
	}
}

void USI_InventoryGridWidget::ResetPressedFeedback(int32 SlotIndex)
{
	if (!SlotWidgets.IsValidIndex(SlotIndex))
	{
		return;
	}

	if (USI_InventorySlotWidget* SlotWidget = SlotWidgets[SlotIndex])
	{
		SlotWidget->ResetPressedFeedback();
	}
}
