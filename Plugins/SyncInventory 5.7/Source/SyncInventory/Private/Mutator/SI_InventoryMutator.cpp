#include "Mutator/SI_InventoryMutator.h"

#include "Component/SI_InventoryComponent.h"

FSI_AddItemResponse FSI_InventoryMutator::AddItem(FSI_InventoryEntryArray& Inventory, const FSI_AddItemRequest& Request,
                                               const int32 ContainerMaxSlots, const int32 ItemMaxStackSize, const USI_InventoryComponent* InventoryComponent)
{
	FSI_AddItemResponse Response;
	Response.Result = ESI_AddItemResult::Failed;
	Response.RequestedQuantity = Request.Quantity;
	Response.AddedQuantity = 0;
	Response.RemainingQuantity = Request.Quantity;
	
	if (!Request.ContainerId.IsValid()) return Response;
	if (!Request.ItemTag.IsValid()) return Response;
	if (Request.Quantity <= 0) return Response;
	if (ContainerMaxSlots <= 0) return Response;
	if (ItemMaxStackSize <= 0) return Response;
	
	int32 Remaining = Response.RemainingQuantity;
	
	// First pass: Fill existing stack of the same item
	for (FSI_InventoryEntry& Entry : Inventory.Entries)
	{
		if (Remaining <= 0) break;
		
		if (Entry.bIsReference) continue;
		if (Entry.ContainerId != Request.ContainerId) continue;
		if (Entry.ItemTag != Request.ItemTag) continue;
		if (Entry.Quantity >= ItemMaxStackSize) continue;
		
		const int32 SpaceLeft = ItemMaxStackSize - Entry.Quantity;
		const int32 ToAdd = FMath::Min(SpaceLeft, Remaining);
		if (ToAdd <= 0) continue;
		
		// Increase the stack
		Entry.Quantity += ToAdd;
		// Reduce what is still left to place
		Remaining -= ToAdd;
		// Record how much the operation successfully added
		Response.AddedQuantity += ToAdd;
		
		// Make it explicit that this specific new entry changed.
		Inventory.MarkItemDirty(Entry);
		
		Response.SlotsToUpdate.AddUnique(Entry.SlotIndex);
	}
	
	// Second pass: Create new stack in free slot
	while (Remaining > 0)
	{
		if (!InventoryComponent) break;

		// Find the first empty slot that also accepts this item.
		const int32 FreeSlotIndex = InventoryComponent->FindFirstFreeAllowedSlot(Request.ContainerId, Request.ItemTag);

		// No valid empty slot exists, so stop trying to add new stacks.
		if (FreeSlotIndex == INDEX_NONE) break;
		
		/* If remaining is smaller than max stack, use all of it
		 * otherwise create a full stack and keep looping for the rest */
		const int32 NewQuantity = FMath::Min(Remaining, ItemMaxStackSize);
		
		FSI_InventoryEntry NewEntry;
		NewEntry.ContainerId = Request.ContainerId;
		NewEntry.ItemTag = Request.ItemTag;
		NewEntry.Quantity = NewQuantity;
		NewEntry.SlotIndex = FreeSlotIndex;
		NewEntry.ItemInstanceData = Request.ItemInstanceData;
		if (!NewEntry.ItemInstanceData.SourceItemTag.IsValid())
		{
			NewEntry.ItemInstanceData.SourceItemTag = Request.ItemTag;
		}
		
		Remaining -= NewQuantity;
		Response.AddedQuantity += NewQuantity;
		
		// We want a reference to the actual stored entry so we can mark it dirty for replication.
		FSI_InventoryEntry& AddedRef = Inventory.Entries.Add_GetRef(NewEntry);
		
		// Make it explicit that this specific new entry changed.
		Inventory.MarkItemDirty(AddedRef);
		
		// Adding or removing entries is a structural change to the FastArray, so MarkArrayDirty() is required.
		Inventory.MarkArrayDirty();
		
		Response.SlotsToUpdate.AddUnique(FreeSlotIndex);
	}
	
	Response.RemainingQuantity = Remaining;
	
	if (Response.AddedQuantity == 0)
	{
		Response.Result = ESI_AddItemResult::Failed;
	}
	else if (Response.RemainingQuantity > 0)
	{
		Response.Result = ESI_AddItemResult::PartialSuccess;
	}
	else
	{
		Response.Result = ESI_AddItemResult::FullSuccess;
	}
	
	return Response;
}

FSI_RemoveItemResponse FSI_InventoryMutator::RemoveItem(FSI_InventoryEntryArray& Inventory, const FSI_RemoveItemRequest& Request) const
{
	FSI_RemoveItemResponse Response;
	Response.Result = ESI_RemoveItemResult::Failed;
	Response.RequestedQuantity = Request.Quantity;
	Response.RemovedQuantity = 0;
	Response.RemainingQuantity = Request.Quantity;
	Response.RemainingInSlot = 0;
	
	if (!Request.ContainerId.IsValid()) return Response;
	if (Request.SlotIndex < 0) return Response;
	if (Request.Quantity <= 0) return Response;
	
	const int32 EntryIndex = FindEntryIndexBySlot(Inventory, Request.ContainerId, Request.SlotIndex);
	if (EntryIndex == INDEX_NONE) return Response;
	
	FSI_InventoryEntry& Entry = Inventory.Entries[EntryIndex];
	const int32 Removed = FMath::Min(Request.Quantity, Entry.Quantity);
	if (Removed <= 0) return Response;
	
	Entry.Quantity -= Removed;
	
	Response.RemovedQuantity = Removed;
	Response.RemainingQuantity = Request.Quantity - Removed;
	Response.RemainingInSlot = Entry.Quantity;
	Response.SlotsToUpdate.AddUnique(Request.SlotIndex);
	
	if (Entry.Quantity <= 0)
	{
		Inventory.Entries.RemoveAt(EntryIndex);
		Inventory.MarkArrayDirty();
	}
	else
	{
		Inventory.MarkItemDirty(Entry);
	}
	
	if (Response.RemovedQuantity == 0)
	{
		Response.Result = ESI_RemoveItemResult::Failed;
	}
	else if (Response.RemainingQuantity > 0)
	{
		Response.Result = ESI_RemoveItemResult::PartialSuccess;
	}
	else
	{
		Response.Result = ESI_RemoveItemResult::FullSuccess;
	}
	
	return Response;
}

FSI_MoveItemResponse FSI_InventoryMutator::MoveItem(FSI_InventoryEntryArray& Inventory, const FGameplayTag& FromContainerId,
	const int32 FromSlotIndex, const FGameplayTag& ToContainerId, const int32 ToSlotIndex,
	const int32 ItemMaxStackSize) const
{
	FSI_MoveItemResponse Response;
	Response.Result = ESI_MoveItemResult::Failed;
	Response.FromContainerId = FromContainerId;
	Response.FromSlotIndex = FromSlotIndex;
	Response.ToContainerId = ToContainerId;
	Response.ToSlotIndex = ToSlotIndex;
	
	// Reject invalid move requests
	if (!FromContainerId.IsValid()) return Response;
	if (!ToContainerId.IsValid()) return Response;
	if (FromSlotIndex < 0) return Response;
	if (ToSlotIndex < 0) return Response;
	if (ItemMaxStackSize <= 0) return Response;
	
	if (FromContainerId == ToContainerId && FromSlotIndex == ToSlotIndex) return Response;
	
	const int32 FromIndex = FindEntryIndexBySlot(Inventory, FromContainerId, FromSlotIndex);
	if (FromIndex == INDEX_NONE) return Response;
	FSI_InventoryEntry& FromEntry = Inventory.Entries[FromIndex];
	
	const int32 ToIndex = FindEntryIndexBySlot(Inventory, ToContainerId, ToSlotIndex);
	// Move to empty slot
	if (ToIndex == INDEX_NONE)
	{
		FromEntry.ContainerId = ToContainerId;
		FromEntry.SlotIndex = ToSlotIndex;
		
		Response.Result = ESI_MoveItemResult::Moved;
		Response.SlotsToUpdate.AddUnique(FromSlotIndex);
		Response.SlotsToUpdate.AddUnique(ToSlotIndex);
		
		Inventory.MarkItemDirty(FromEntry);
		return Response;
	}
	
	FSI_InventoryEntry& ToEntry = Inventory.Entries[ToIndex];
	
	// References are shortcuts, not owned stacks.
	// Reference-to-reference drops should fall through to the swap logic below.
	const bool bEitherEntryIsReference = FromEntry.bIsReference || ToEntry.bIsReference;

	// Merge real stacks only when neither side is a reference.
	if (!bEitherEntryIsReference && FromEntry.ItemTag == ToEntry.ItemTag && ToEntry.Quantity < ItemMaxStackSize)
	{
		const int32 SpaceLeft = ItemMaxStackSize - ToEntry.Quantity;
		const int32 TransferQuantity = FMath::Min(SpaceLeft, FromEntry.Quantity);
		
		if (TransferQuantity <= 0) return Response;
		
		ToEntry.Quantity += TransferQuantity;
		FromEntry.Quantity -= TransferQuantity;
		
		Response.Result = ESI_MoveItemResult::Merged;
		Response.SlotsToUpdate.AddUnique(FromSlotIndex);
		Response.SlotsToUpdate.AddUnique(ToSlotIndex);
		
		Inventory.MarkItemDirty(ToEntry);
		
		if (FromEntry.Quantity <= 0)
		{
			Inventory.Entries.RemoveAt(FromIndex);
			Inventory.MarkArrayDirty();
		}
		else
		{
			Inventory.MarkItemDirty(FromEntry);
		}
		
		return Response;
	}
	
	// Otherwise swap the two occupied slots
	const FGameplayTag OldFromContainerId = FromEntry.ContainerId;
	const int32 OldFromSlotIndex = FromEntry.SlotIndex;
	FromEntry.ContainerId = ToEntry.ContainerId;
	FromEntry.SlotIndex = ToEntry.SlotIndex;
	ToEntry.ContainerId = OldFromContainerId;
	ToEntry.SlotIndex = OldFromSlotIndex;
	
	Response.Result = ESI_MoveItemResult::Swapped;
	Response.SlotsToUpdate.AddUnique(FromSlotIndex);
	Response.SlotsToUpdate.AddUnique(ToSlotIndex);
	
	Inventory.MarkItemDirty(FromEntry);
	Inventory.MarkItemDirty(ToEntry);
	
	return Response;
}

FSI_SplitStackResponse FSI_InventoryMutator::SplitStack(FSI_InventoryEntryArray& Inventory, const FGameplayTag& ContainerId,
	const int32 SlotIndex, int32 SplitQuantity, int32 MaxSlots, const USI_InventoryComponent* InventoryComponent) const
{
	FSI_SplitStackResponse Response;
	Response.Result = ESI_SplitStackResult::Failed;
	Response.ContainerId = ContainerId;
	Response.SlotIndex = SlotIndex;
	Response.NewSlotIndex = INDEX_NONE;
	Response.SplitQuantity = 0;
	
	// Reject invalid split request
	if (!ContainerId.IsValid()) return Response;
	if (SlotIndex < 0) return Response;
	if (SplitQuantity <= 0) return Response;
	if (MaxSlots <= 0) return Response;
	
	const int32 SourceIndex = FindEntryIndexBySlot(Inventory, ContainerId, SlotIndex);
	if (SourceIndex == INDEX_NONE) return Response;
	
	FSI_InventoryEntry& SourceEntry = Inventory.Entries[SourceIndex];
	if (SourceEntry.bIsReference) return Response;
	if (SourceEntry.Quantity <= 1) return Response;
	
	// You cannot split the whole stack or more than the stack contains.
	if (SplitQuantity >= SourceEntry.Quantity) return Response;
	
	if (!InventoryComponent) return Response;

	// Find an empty slot that also accepts this same item type.
	// Splitting should not place the new stack into a restricted slot.
	const int32 NewSlotIndex = InventoryComponent->FindFirstFreeAllowedSlot(ContainerId, SourceEntry.ItemTag);

	// If no valid empty slot exists, the split cannot happen.
	if (NewSlotIndex == INDEX_NONE) return Response;
	
	FSI_InventoryEntry NewEntry;
	NewEntry.ContainerId = ContainerId;
	NewEntry.SlotIndex = NewSlotIndex;
	NewEntry.ItemTag = SourceEntry.ItemTag;
	NewEntry.Quantity = SplitQuantity;
	NewEntry.ItemInstanceData = SourceEntry.ItemInstanceData;
	
	SourceEntry.Quantity -= SplitQuantity;
	
	Response.Result = ESI_SplitStackResult::Success;
	Response.NewSlotIndex = NewSlotIndex;
	Response.SplitQuantity = SplitQuantity;
	Response.SlotsToUpdate.AddUnique(SlotIndex);
	Response.SlotsToUpdate.AddUnique(NewSlotIndex);
	
	Inventory.MarkItemDirty(SourceEntry);
	
	FSI_InventoryEntry& AddedRef = Inventory.Entries.Add_GetRef(NewEntry);
	Inventory.MarkItemDirty(AddedRef);
	Inventory.MarkArrayDirty();
	
	return Response;
}

FSI_MoveItemResponse FSI_InventoryMutator::CreateReferenceEntry(FSI_InventoryEntryArray& Inventory,
	const FSI_MoveItemRequest& Request, const FGameplayTag& SourceContainerId, int32 SourceSlotIndex,
	const FGameplayTag& SourceItemTag, const FSI_ItemInstanceData& SourceItemInstanceData) const
{
	FSI_MoveItemResponse Response;
	Response.Result = ESI_MoveItemResult::Failed;
	Response.FromContainerId = Request.FromContainerId;
	Response.FromSlotIndex = Request.FromSlotIndex;
	Response.ToContainerId = Request.ToContainerId;
	Response.ToSlotIndex = Request.ToSlotIndex;

	if (!Request.ToContainerId.IsValid()) return Response;
	if (Request.ToSlotIndex < 0) return Response;
	if (!SourceContainerId.IsValid()) return Response;
	if (SourceSlotIndex < 0) return Response;
	if (!SourceItemTag.IsValid()) return Response;

	const int32 ExistingToIndex = FindEntryIndexBySlot(Inventory, Request.ToContainerId, Request.ToSlotIndex);

	if (ExistingToIndex != INDEX_NONE)
	{
		const FSI_InventoryEntry& ExistingToEntry = Inventory.Entries[ExistingToIndex];

		// Reference creation replaces shortcuts only.
		// This avoids deleting a real item if bad data ever gets into the skillbar.
		if (!ExistingToEntry.bIsReference) return Response;

		Inventory.Entries.RemoveAt(ExistingToIndex);
		Inventory.MarkArrayDirty();
	}

	FSI_InventoryEntry NewEntry;
	NewEntry.ContainerId = Request.ToContainerId;
	NewEntry.SlotIndex = Request.ToSlotIndex;
	NewEntry.ItemTag = SourceItemTag;

	// Reference entries do not own item quantity.
	// UI will later display the source inventory's aggregated quantity.
	NewEntry.Quantity = 1;

	NewEntry.bIsReference = true;
	NewEntry.SourceContainerId = SourceContainerId;
	NewEntry.SourceSlotIndex = SourceSlotIndex;
	NewEntry.ItemInstanceData = SourceItemInstanceData;
	if (!NewEntry.ItemInstanceData.SourceItemTag.IsValid())
	{
		NewEntry.ItemInstanceData.SourceItemTag = SourceItemTag;
	}

	FSI_InventoryEntry& AddedRef = Inventory.Entries.Add_GetRef(NewEntry);

	// Mark both the new item and the array structure for replication.
	Inventory.MarkItemDirty(AddedRef);
	Inventory.MarkArrayDirty();

	Response.Result = ESI_MoveItemResult::Moved;
	Response.SlotsToUpdate.AddUnique(Request.FromSlotIndex);
	Response.SlotsToUpdate.AddUnique(Request.ToSlotIndex);

	return Response;
}

int32 FSI_InventoryMutator::FindFirstFreeSlot(const FSI_InventoryEntryArray& Inventory, const FGameplayTag& ContainerId,
                                              int32 MaxSlots)
{
	if (!ContainerId.IsValid() || MaxSlots <= 0) return INDEX_NONE;
	
	// Create a bit array to tract which slot indices are already used in the container.
	// It starts with all slots set to false, meaning all slots are considered empty at first.
	TBitArray<> UsedSlots(false, MaxSlots);
	
	// Check every inventory entry to see which slots are already used
	for (const FSI_InventoryEntry& Entry : Inventory.Entries)
	{
		// Only mark slots that belong to this container and are in valid range
		if (Entry.ContainerId == ContainerId && Entry.SlotIndex >= 0 && Entry.SlotIndex < MaxSlots)
		{
			UsedSlots[Entry.SlotIndex] = true;
		}
	}
	
	// Scan from the start and return the first slot that is still free
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; SlotIndex++)
	{
		if (UsedSlots[SlotIndex]) continue;
		
		return SlotIndex;
	}
	
	return INDEX_NONE;
}

int32 FSI_InventoryMutator::FindEntryIndexBySlot(const FSI_InventoryEntryArray& Inventory, const FGameplayTag& ContainerId,
	int32 SlotIndex)
{
	// Look for the entry that matches this container and slot
	for (int32 Index = 0; Index < Inventory.Entries.Num(); ++Index)
	{
		const FSI_InventoryEntry& Entry = Inventory.Entries[Index];
		
		// Return the array index of the matching entry
		if (Entry.ContainerId == ContainerId && Entry.SlotIndex == SlotIndex)
		{
			return Index;
		}
	}
	
	return INDEX_NONE;
}
