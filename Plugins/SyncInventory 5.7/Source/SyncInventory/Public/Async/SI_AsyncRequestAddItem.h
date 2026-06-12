#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/Requests/SI_RequestTypes.h"
#include "SI_AsyncRequestAddItem.generated.h"

class USI_InventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSI_AsyncAddItemCompleted, const FSI_AddItemResponse&, Response,
	bool, bSuccess);

UCLASS()
class SYNCINVENTORY_API USI_AsyncRequestAddItem : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Inventory|Async", meta=(BlueprintInternalUseOnly="true"))
	static USI_AsyncRequestAddItem* AsyncRequestAddItem(USI_InventoryComponent* InventoryComponent,
		FSI_AddItemRequest Request);

	UPROPERTY(BlueprintAssignable)
	FSI_AsyncAddItemCompleted Completed;

	virtual void Activate() override;
	
private:
	UFUNCTION()
	void HandleAddItemCompleted(const FSI_AddItemResponse& Response);

	void Finish(const FSI_AddItemResponse& Response);
	void Cleanup();

	UPROPERTY()
	TObjectPtr<USI_InventoryComponent> InventoryComponent = nullptr;

	UPROPERTY()
	FSI_AddItemRequest Request;

	UPROPERTY()
	int32 RequestId = INDEX_NONE;

	UPROPERTY()
	bool bHasFinished = false;
};
