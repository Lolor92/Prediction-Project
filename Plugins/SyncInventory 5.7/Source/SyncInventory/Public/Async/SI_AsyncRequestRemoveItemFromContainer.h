#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/Requests/SI_RequestTypes.h"
#include "SI_AsyncRequestRemoveItemFromContainer.generated.h"

class USI_InventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSI_AsyncRemoveItemFromContainerCompleted,
	const FSI_RemoveItemResponse&, Response, bool, bSuccess);

UCLASS()
class SYNCINVENTORY_API USI_AsyncRequestRemoveItemFromContainer : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Inventory|Async", meta=(BlueprintInternalUseOnly="true"))
	static USI_AsyncRequestRemoveItemFromContainer* AsyncRequestRemoveItemFromContainer(
		USI_InventoryComponent* InventoryComponent,
		FSI_RemoveItemFromContainerRequest Request);

	UPROPERTY(BlueprintAssignable)
	FSI_AsyncRemoveItemFromContainerCompleted Completed;

	virtual void Activate() override;
	
private:
	UFUNCTION()
	void HandleRemoveItemCompleted(const FSI_RemoveItemResponse& Response);

	void Finish(const FSI_RemoveItemResponse& Response);
	void Cleanup();

	UPROPERTY()
	TObjectPtr<USI_InventoryComponent> InventoryComponent = nullptr;

	UPROPERTY()
	FSI_RemoveItemFromContainerRequest Request;

	UPROPERTY()
	int32 RequestId = INDEX_NONE;

	UPROPERTY()
	bool bHasFinished = false;
};
