#pragma once

#include "CoreMinimal.h"
#include "Replication/SI_InventoryFastArray.h"
#include "Types/Requests/SI_RequestTypes.h"

class SYNCINVENTORY_API FSI_InventoryMutator
{
public:
	FSI_AddItemResponse AddItem(FSI_InventoryEntryArray& Inventory, const FSI_AddItemRequest& Request,
		const int32 ContainerMaxSlots, const int32 ItemMaxStackSize, const USI_InventoryComponent* InventoryComponent);
	FSI_RemoveItemResponse RemoveItem(FSI_InventoryEntryArray& Inventory, const FSI_RemoveItemRequest& Request) const;
	FSI_MoveItemResponse MoveItem(FSI_InventoryEntryArray& Inventory, const FGameplayTag& FromContainerId, const int32 FromSlotIndex, 
		const FGameplayTag& ToContainerId, const int32 ToSlotIndex, const int32 ItemMaxStackSize) const;
	FSI_SplitStackResponse SplitStack(FSI_InventoryEntryArray& Inventory, const FGameplayTag& ContainerId, const int32 SlotIndex,
		int32 SplitQuantity, int32 MaxSlots, const USI_InventoryComponent* InventoryComponent) const;
	FSI_MoveItemResponse CreateReferenceEntry(FSI_InventoryEntryArray& Inventory, const FSI_MoveItemRequest& Request,
		const FGameplayTag& SourceContainerId, int32 SourceSlotIndex, const FGameplayTag& SourceItemTag,
		const FSI_ItemInstanceData& SourceItemInstanceData) const;
	
	static int32 FindFirstFreeSlot(const FSI_InventoryEntryArray& Inventory, const FGameplayTag& ContainerId,
		int32 MaxSlots);
	static int32 FindEntryIndexBySlot(const FSI_InventoryEntryArray& Inventory, const FGameplayTag& ContainerId,
		int32 SlotIndex);
};
