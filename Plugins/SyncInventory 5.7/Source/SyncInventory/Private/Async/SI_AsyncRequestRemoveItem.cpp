#include "Async/SI_AsyncRequestRemoveItem.h"
#include "Component/SI_InventoryComponent.h"

USI_AsyncRequestRemoveItem* USI_AsyncRequestRemoveItem::AsyncRequestRemoveItem(
	USI_InventoryComponent* InventoryComponent, FSI_RemoveItemRequest Request)
{
	USI_AsyncRequestRemoveItem* Action = NewObject<USI_AsyncRequestRemoveItem>();
	Action->InventoryComponent = InventoryComponent;
	Action->Request = Request;

	return Action;
}

void USI_AsyncRequestRemoveItem::Activate()
{
	if (!InventoryComponent)
	{
		FSI_RemoveItemResponse Response;
		Response.RequestId = Request.RequestId;
		Response.ContainerId = Request.ContainerId;
		Response.SlotIndex = Request.SlotIndex;
		Response.Result = ESI_RemoveItemResult::Failed;
		Response.RequestedQuantity = Request.Quantity;
		Response.RemainingQuantity = Request.Quantity;

		Finish(Response);
		return;
	}
	
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = InventoryComponent->GenerateRequestId();
	}

	RequestId = Request.RequestId;

	InventoryComponent->OnRemoveItemRequestCompleted.AddDynamic(this, &ThisClass::HandleRemoveItemCompleted);
	InventoryComponent->RequestRemoveItem(Request);
}

void USI_AsyncRequestRemoveItem::HandleRemoveItemCompleted(const FSI_RemoveItemResponse& Response)
{
	if (Response.RequestId != RequestId) return;

	Finish(Response);
}

void USI_AsyncRequestRemoveItem::Finish(const FSI_RemoveItemResponse& Response)
{
	if (bHasFinished) return;

	bHasFinished = true;

	const bool bSuccess = Response.Result != ESI_RemoveItemResult::Failed
		&& Response.RemovedQuantity > 0;

	Completed.Broadcast(Response, bSuccess);

	Cleanup();
	SetReadyToDestroy();
}

void USI_AsyncRequestRemoveItem::Cleanup()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnRemoveItemRequestCompleted.RemoveDynamic(
			this, &ThisClass::HandleRemoveItemCompleted);
	}
}
