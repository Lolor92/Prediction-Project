#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/Requests/SI_RequestTypes.h"
#include "SI_AsyncRequestSplitItem.generated.h"

class USI_InventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSI_AsyncSplitItemCompleted, const FSI_SplitStackResponse&, Response,
	bool, bSuccess);

UCLASS()
class SYNCINVENTORY_API USI_AsyncRequestSplitItem : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Inventory|Async", meta=(BlueprintInternalUseOnly="true"))
	static USI_AsyncRequestSplitItem* AsyncRequestSplitItem(USI_InventoryComponent* InventoryComponent,
		FSI_SplitStackRequest Request);

	UPROPERTY(BlueprintAssignable)
	FSI_AsyncSplitItemCompleted Completed;

	virtual void Activate() override;
	
private:
	UFUNCTION()
	void HandleSplitItemCompleted(const FSI_SplitStackResponse& Response);

	void Finish(const FSI_SplitStackResponse& Response);
	void Cleanup();

	UPROPERTY()
	TObjectPtr<USI_InventoryComponent> InventoryComponent = nullptr;

	UPROPERTY()
	FSI_SplitStackRequest Request;

	UPROPERTY()
	int32 RequestId = INDEX_NONE;

	UPROPERTY()
	bool bHasFinished = false;
};
