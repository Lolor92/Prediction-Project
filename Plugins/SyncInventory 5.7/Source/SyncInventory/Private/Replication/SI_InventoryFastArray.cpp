#include "Replication/SI_InventoryFastArray.h"
#include "Component/SI_InventoryComponent.h"


void FSI_InventoryEntryArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (InventoryComponent)
	{
		InventoryComponent->HandleReplicationAdd(AddedIndices);
	}
}

void FSI_InventoryEntryArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	PendingRemovedEntries.Reset();
	
	for (int32 Index : RemovedIndices)
	{
		if (Entries.IsValidIndex(Index))
		{
			PendingRemovedEntries.Add(Entries[Index]);
		}
	}
}

void FSI_InventoryEntryArray::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (InventoryComponent)
	{
		InventoryComponent->HandleReplicationChange(ChangedIndices);
	}
}

void FSI_InventoryEntryArray::PostReplicatedReceive(const FPostReplicatedReceiveParameters& Parameters)
{
	if (!InventoryComponent) return;
	
	if (PendingRemovedEntries.Num() > 0)
	{
		InventoryComponent->HandleReplicationRemove(PendingRemovedEntries);
		PendingRemovedEntries.Reset();
	}
	
	InventoryComponent->FlushPendingClientSlotRefreshes();
}
