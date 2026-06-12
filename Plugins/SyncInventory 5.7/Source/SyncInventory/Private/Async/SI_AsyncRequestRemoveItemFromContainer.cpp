#include "Async/SI_AsyncRequestRemoveItemFromContainer.h"
#include "Component/SI_InventoryComponent.h"

USI_AsyncRequestRemoveItemFromContainer* USI_AsyncRequestRemoveItemFromContainer::AsyncRequestRemoveItemFromContainer(
	USI_InventoryComponent* InventoryComponent, FSI_RemoveItemFromContainerRequest Request)
{
	USI_AsyncRequestRemoveItemFromContainer* Action = NewObject<USI_AsyncRequestRemoveItemFromContainer>();
	Action->InventoryComponent = InventoryComponent;
	Action->Request = Request;

	return Action;
}

void USI_AsyncRequestRemoveItemFromContainer::Activate()
{
	if (!InventoryComponent)
	{
		FSI_RemoveItemResponse Response;
		Response.RequestId = Request.RequestId;
		Response.ContainerId = Request.ContainerId;
		Response.ItemTag = Request.ItemTag;
		Response.Result = ESI_RemoveItemResult::Failed;
		Response.RequestedQuantity = Request.bRemoveAll ? 0 : Request.Quantity;
		Response.RemainingQuantity = Response.RequestedQuantity;

		Finish(Response);
		return;
	}
	
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = InventoryComponent->GenerateRequestId();
	}

	RequestId = Request.RequestId;

	InventoryComponent->OnRemoveItemRequestCompleted.AddDynamic(this, &ThisClass::HandleRemoveItemCompleted);
	InventoryComponent->RequestRemoveItemFromContainer(Request);
}

void USI_AsyncRequestRemoveItemFromContainer::HandleRemoveItemCompleted(const FSI_RemoveItemResponse& Response)
{
	if (Response.RequestId != RequestId) return;

	Finish(Response);
}

void USI_AsyncRequestRemoveItemFromContainer::Finish(const FSI_RemoveItemResponse& Response)
{
	if (bHasFinished) return;

	bHasFinished = true;

	const bool bSuccess = Response.Result != ESI_RemoveItemResult::Failed
		&& Response.RemovedQuantity > 0;

	Completed.Broadcast(Response, bSuccess);

	Cleanup();
	SetReadyToDestroy();
}

void USI_AsyncRequestRemoveItemFromContainer::Cleanup()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnRemoveItemRequestCompleted.RemoveDynamic(
			this, &ThisClass::HandleRemoveItemCompleted);
	}
}
