#include "Async/SI_AsyncRequestAddItem.h"
#include "Component/SI_InventoryComponent.h"

USI_AsyncRequestAddItem* USI_AsyncRequestAddItem::AsyncRequestAddItem(USI_InventoryComponent* InventoryComponent,
	FSI_AddItemRequest Request)
{
	USI_AsyncRequestAddItem* Action = NewObject<USI_AsyncRequestAddItem>();
	Action->InventoryComponent = InventoryComponent;
	Action->Request = Request;

	return Action;
}

void USI_AsyncRequestAddItem::Activate()
{
	if (!InventoryComponent)
	{
		FSI_AddItemResponse Response;
		Response.RequestId = Request.RequestId;
		Response.ContainerId = Request.ContainerId;
		Response.ItemTag = Request.ItemTag;
		Response.Result = ESI_AddItemResult::Failed;
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

	InventoryComponent->OnAddItemRequestCompleted.AddDynamic(this, &ThisClass::HandleAddItemCompleted);
	InventoryComponent->RequestAddItem(Request);
}

void USI_AsyncRequestAddItem::HandleAddItemCompleted(const FSI_AddItemResponse& Response)
{
	if (Response.RequestId != RequestId) return;

	Finish(Response);
}

void USI_AsyncRequestAddItem::Finish(const FSI_AddItemResponse& Response)
{
	if (bHasFinished) return;

	bHasFinished = true;

	const bool bSuccess = Response.Result != ESI_AddItemResult::Failed
		&& Response.AddedQuantity > 0;

	Completed.Broadcast(Response, bSuccess);

	Cleanup();
	SetReadyToDestroy();
}

void USI_AsyncRequestAddItem::Cleanup()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnAddItemRequestCompleted.RemoveDynamic(
			this, &ThisClass::HandleAddItemCompleted);
	}
}
