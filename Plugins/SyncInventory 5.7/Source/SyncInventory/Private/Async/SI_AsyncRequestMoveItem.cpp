#include "Async/SI_AsyncRequestMoveItem.h"
#include "Component/SI_InventoryComponent.h"

USI_AsyncRequestMoveItem* USI_AsyncRequestMoveItem::AsyncRequestMoveItem(
	USI_InventoryComponent* InventoryComponent, FSI_MoveItemRequest Request)
{
	USI_AsyncRequestMoveItem* Action = NewObject<USI_AsyncRequestMoveItem>();
	Action->InventoryComponent = InventoryComponent;
	Action->Request = Request;

	return Action;
}

void USI_AsyncRequestMoveItem::Activate()
{
	if (!InventoryComponent)
	{
		FSI_MoveItemResponse Response;
		Response.RequestId = Request.RequestId;
		Response.FromContainerId = Request.FromContainerId;
		Response.FromSlotIndex = Request.FromSlotIndex;
		Response.ToContainerId = Request.ToContainerId;
		Response.ToSlotIndex = Request.ToSlotIndex;
		Response.Result = ESI_MoveItemResult::Failed;

		Finish(Response);
		return;
	}
	
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = InventoryComponent->GenerateRequestId();
	}

	RequestId = Request.RequestId;

	InventoryComponent->OnMoveItemRequestCompleted.AddDynamic(this, &ThisClass::HandleMoveItemCompleted);
	InventoryComponent->RequestMoveItem(Request);
}

void USI_AsyncRequestMoveItem::HandleMoveItemCompleted(const FSI_MoveItemResponse& Response)
{
	if (Response.RequestId != RequestId) return;

	Finish(Response);
}

void USI_AsyncRequestMoveItem::Finish(const FSI_MoveItemResponse& Response)
{
	if (bHasFinished) return;

	bHasFinished = true;

	const bool bSuccess = Response.Result != ESI_MoveItemResult::Failed;

	Completed.Broadcast(Response, bSuccess);

	Cleanup();
	SetReadyToDestroy();
}

void USI_AsyncRequestMoveItem::Cleanup()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnMoveItemRequestCompleted.RemoveDynamic(
			this, &ThisClass::HandleMoveItemCompleted);
	}
}
