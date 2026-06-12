#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/DragDropOperation.h"
#include "SI_InventoryDragOperation.generated.h"


class USI_InventoryComponent;

UCLASS()
class SYNCINVENTORY_API USI_InventoryDragOperation : public UDragDropOperation
{
	GENERATED_BODY()
	
public:
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_InventoryComponent> InventoryComponent = nullptr;
	
	UPROPERTY(Transient)
	FGameplayTag ContainerId;
	
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;
	
	// Whether dropping into empty UI space can trigger a delete prompt
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Inventory")
	bool bAllowDeleteOnEmptyDrop = false;
};
