#include "UI/DragDrop/SI_InventoryDragOperation.h"
#include "Component/SI_InventoryComponent.h"
#include "Component/SI_InventoryUIManager.h"

void USI_InventoryDragOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);
	
	if (!InventoryComponent || !ContainerId.IsValid() || SlotIndex < 0)
	{
		return;
	}

	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry)
	{
		return;
	}

	// Dragging a reference out clears only the shortcut, never the real item.
	if (Entry->bIsReference)
	{
		FSI_RemoveItemRequest Request;
		Request.ContainerId = ContainerId;
		Request.SlotIndex = SlotIndex;
		Request.Quantity = 1;

		InventoryComponent->RequestRemoveItem(Request);
		return;
	}

	// Real items still use the normal delete confirmation flow.
	if (!bAllowDeleteOnEmptyDrop)
	{
		return;
	}

	AActor* OwnerActor = InventoryComponent->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (USI_InventoryUIManager* UIManager = OwnerActor->FindComponentByClass<USI_InventoryUIManager>())
	{
		UIManager->OpenDeleteItemWidget(ContainerId, SlotIndex, PointerEvent.GetScreenSpacePosition());
	}
}
