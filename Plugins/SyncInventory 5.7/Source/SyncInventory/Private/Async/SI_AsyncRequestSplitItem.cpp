#include "Async/SI_AsyncRequestSplitItem.h"
#include "Component/SI_InventoryComponent.h"

USI_AsyncRequestSplitItem* USI_AsyncRequestSplitItem::AsyncRequestSplitItem(
	USI_InventoryComponent* InventoryComponent, FSI_SplitStackRequest Request)
{
	USI_AsyncRequestSplitItem* Action = NewObject<USI_AsyncRequestSplitItem>();
	Action->InventoryComponent = InventoryComponent;
	Action->Request = Request;

	return Action;
}

void USI_AsyncRequestSplitItem::Activate()
{
	if (!InventoryComponent)
	{
		FSI_SplitStackResponse Response;
		Response.RequestId = Request.RequestId;
		Response.ContainerId = Request.ContainerId;
		Response.SlotIndex = Request.SlotIndex;
		Response.SplitQuantity = Request.SplitQuantity;
		Response.Result = ESI_SplitStackResult::Failed;

		Finish(Response);
		return;
	}
	
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = InventoryComponent->GenerateRequestId();
	}

	RequestId = Request.RequestId;

	InventoryComponent->OnSplitItemRequestCompleted.AddDynamic(this, &ThisClass::HandleSplitItemCompleted);
	InventoryComponent->RequestSplitItem(Request);
}

void USI_AsyncRequestSplitItem::HandleSplitItemCompleted(const FSI_SplitStackResponse& Response)
{
	if (Response.RequestId != RequestId) return;

	Finish(Response);
}

void USI_AsyncRequestSplitItem::Finish(const FSI_SplitStackResponse& Response)
{
	if (bHasFinished) return;

	bHasFinished = true;

	const bool bSuccess = Response.Result == ESI_SplitStackResult::Success;

	Completed.Broadcast(Response, bSuccess);

	Cleanup();
	SetReadyToDestroy();
}

void USI_AsyncRequestSplitItem::Cleanup()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnSplitItemRequestCompleted.RemoveDynamic(
			this, &ThisClass::HandleSplitItemCompleted);
	}
}
