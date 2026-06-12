#include "UI/Popups/SI_DeleteItemWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Component/SI_InventoryComponent.h"
#include "Component/SI_InventoryUIManager.h"
#include "Replication/SI_InventoryFastArray.h"

void USI_DeleteItemWidget::InitDeleteItemWidget(USI_InventoryUIManager* InInventoryUIManager,
                                                USI_InventoryComponent* InInventoryComponent, const FGameplayTag& InContainerId, int32 InSlotIndex)
{
	InventoryUIManager = InInventoryUIManager;
	InventoryComponent = InInventoryComponent;
	ContainerId = InContainerId;
	SlotIndex = InSlotIndex;
	
	Refresh();
}

void USI_DeleteItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (ConfirmDeleteButton)
	{
		ConfirmDeleteButton->OnClicked.RemoveAll(this);
		ConfirmDeleteButton->OnClicked.AddDynamic(this, &ThisClass::HandleConfirmDeleteClicked);
	}
	
	if (CancelDeleteButton)
	{
		CancelDeleteButton->OnClicked.RemoveAll(this);
		CancelDeleteButton->OnClicked.AddDynamic(this, &ThisClass::HandleCancelDeleteClicked);
	}
	
	Refresh();
}

void USI_DeleteItemWidget::SetPopScreenPosition(const FVector2D& ScreenPosition)
{
	if (!PopupRoot) return;
	
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), ScreenPosition,
		PixelPosition, ViewportPosition);
	
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PopupRoot->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
		CanvasSlot->SetPosition(ViewportPosition);
	}

	CaptureDefaultLayout();
}

void USI_DeleteItemWidget::RequestClose()
{
	if (InventoryUIManager.IsValid())
	{
		InventoryUIManager->CloseDeleteItemWidget();
		return;
	}
	
	RemoveFromParent();
}

void USI_DeleteItemWidget::Refresh()
{
	if (!InventoryComponent.IsValid()) return;
	
	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry) return;
	
	const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
	if (!ItemDatabase) return;
	
	const USI_ItemDefinition* Def = ItemDatabase->GetItemDefinitionByTag(Entry->ItemTag);
	if (!Def) return;
	
	if (ItemNameText)
	{
		ItemNameText->SetText(Def->ItemName);
	}
	if (QuantityText)
	{
		QuantityText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Entry->Quantity)));
	}
}

UWidget* USI_DeleteItemWidget::GetManipulationTarget() const
{
	return DragHandle ? DragHandle.Get() : Cast<UWidget>(PopupRoot);
}

void USI_DeleteItemWidget::HandleConfirmDeleteClicked()
{
	if (!InventoryComponent.IsValid())
	{
		RequestClose();
		return;
	}
	
	if (!ContainerId.IsValid() || SlotIndex < 0)
	{
		RequestClose();
		return;
	}
	
	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry || Entry->Quantity <= 0)
	{
		RequestClose();
		return;
	}
	
	FSI_RemoveItemRequest Request;
	Request.ContainerId = ContainerId;
	Request.SlotIndex = SlotIndex;
	Request.Quantity = Entry->Quantity;
	
	InventoryComponent->RequestRemoveItem(Request);
	RequestClose();
}

void USI_DeleteItemWidget::HandleCancelDeleteClicked()
{
	RequestClose();
}
