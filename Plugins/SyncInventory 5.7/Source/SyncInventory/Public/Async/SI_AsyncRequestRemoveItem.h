#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/Requests/SI_RequestTypes.h"
#include "SI_AsyncRequestRemoveItem.generated.h"

class USI_InventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSI_AsyncRemoveItemCompleted, const FSI_RemoveItemResponse&, Response,
	bool, bSuccess);

UCLASS()
class SYNCINVENTORY_API USI_AsyncRequestRemoveItem : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Inventory|Async", meta=(BlueprintInternalUseOnly="true"))
	static USI_AsyncRequestRemoveItem* AsyncRequestRemoveItem(USI_InventoryComponent* InventoryComponent,
		FSI_RemoveItemRequest Request);

	UPROPERTY(BlueprintAssignable)
	FSI_AsyncRemoveItemCompleted Completed;

	virtual void Activate() override;
	
private:
	UFUNCTION()
	void HandleRemoveItemCompleted(const FSI_RemoveItemResponse& Response);

	void Finish(const FSI_RemoveItemResponse& Response);
	void Cleanup();

	UPROPERTY()
	TObjectPtr<USI_InventoryComponent> InventoryComponent = nullptr;

	UPROPERTY()
	FSI_RemoveItemRequest Request;

	UPROPERTY()
	int32 RequestId = INDEX_NONE;

	UPROPERTY()
	bool bHasFinished = false;
};
