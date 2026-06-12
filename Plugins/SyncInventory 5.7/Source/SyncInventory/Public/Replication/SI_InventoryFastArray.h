#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Types/Item/SI_ItemTypes.h"
#include "SI_InventoryFastArray.generated.h"

/* 
A FastArray in Unreal is a replication pattern for arrays that change often.
Instead of replicating the entire array every time one item changes, Unreal can replicate only the changed entries.
That makes it a good fit for inventories, because items are constantly being added, removed, stacked, or moved.
*/

class USI_InventoryComponent;

USTRUCT()
struct FSI_InventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	// ContainerId lets one array hold multiple inventory views.
	// Each entry says which container it belongs to, like Player, Equipment, or SkillBar.
	UPROPERTY()
	FGameplayTag ContainerId;
	
	UPROPERTY()
	FGameplayTag ItemTag;
	
	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;
	
	UPROPERTY()
	bool bIsReference = false;
	
	UPROPERTY()
	FGameplayTag SourceContainerId;
	
	UPROPERTY()
	int32 SourceSlotIndex = INDEX_NONE;
	
	UPROPERTY()
	int32 Quantity = 0;

	UPROPERTY()
	FSI_ItemInstanceData ItemInstanceData;
	
	bool IsValidEntry() const
	{
		return ItemTag.IsValid() && SlotIndex != INDEX_NONE && Quantity > 0;
	}
	
	// Checks whether this reference can resolve back to a real inventory slot.
	bool HasValidSourceReference() const
	{
		return bIsReference && SourceContainerId.IsValid() && SourceSlotIndex != INDEX_NONE;
	}
};

USTRUCT()
struct FSI_InventoryEntryArray : public FFastArraySerializer
{
	GENERATED_BODY()
	
	FSI_InventoryEntryArray() = default;
	FSI_InventoryEntryArray(USI_InventoryComponent* InInventoryComponent) { InventoryComponent = InInventoryComponent; }
	
	UPROPERTY()
	TArray<FSI_InventoryEntry> Entries;
	
	UPROPERTY(NotReplicated)
	TArray<FSI_InventoryEntry> PendingRemovedEntries;
	
	UPROPERTY(NotReplicated)
	TObjectPtr<USI_InventoryComponent> InventoryComponent;
	
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	void PostReplicatedReceive(const FPostReplicatedReceiveParameters& Parameters);
	
	// Tells Unreal to replicate Entries using Fast Array delta serialization.
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FSI_InventoryEntry, FSI_InventoryEntryArray>(Entries, DeltaParams, *this);
	}
};

// NetDeltaSerialize(...) defines how to replicate it
// TStructOpsTypeTraits<...> tells Unreal that this struct actually has that custom serializer
template<>
struct TStructOpsTypeTraits<FSI_InventoryEntryArray> : TStructOpsTypeTraitsBase2<FSI_InventoryEntryArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
		WithPostReplicatedReceive = true
	};
};
