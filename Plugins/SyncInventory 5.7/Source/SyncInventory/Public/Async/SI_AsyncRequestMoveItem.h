#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/Requests/SI_RequestTypes.h"
#include "SI_AsyncRequestMoveItem.generated.h"

class USI_InventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSI_AsyncMoveItemCompleted, const FSI_MoveItemResponse&, Response,
	bool, bSuccess);

UCLASS()
class SYNCINVENTORY_API USI_AsyncRequestMoveItem : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Inventory|Async", meta=(BlueprintInternalUseOnly="true"))
	static USI_AsyncRequestMoveItem* AsyncRequestMoveItem(USI_InventoryComponent* InventoryComponent,
		FSI_MoveItemRequest Request);

	UPROPERTY(BlueprintAssignable)
	FSI_AsyncMoveItemCompleted Completed;

	virtual void Activate() override;
	
private:
	UFUNCTION()
	void HandleMoveItemCompleted(const FSI_MoveItemResponse& Response);

	void Finish(const FSI_MoveItemResponse& Response);
	void Cleanup();

	UPROPERTY()
	TObjectPtr<USI_InventoryComponent> InventoryComponent = nullptr;

	UPROPERTY()
	FSI_MoveItemRequest Request;

	UPROPERTY()
	int32 RequestId = INDEX_NONE;

	UPROPERTY()
	bool bHasFinished = false;
};
