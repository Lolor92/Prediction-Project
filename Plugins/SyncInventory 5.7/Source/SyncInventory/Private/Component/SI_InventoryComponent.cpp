#include "Component/SI_InventoryComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayEffect.h"
#include "Data/Databases/SI_SlotLayoutConfig.h"
#include "Interaction/SI_InventoryInteractor.h"
#include "Net/UnrealNetwork.h"


USI_InventoryComponent::USI_InventoryComponent() : Inventory(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USI_InventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, Inventory);
	DOREPLIFETIME(ThisClass, CharacterLevel);
}

void USI_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	InventoryInteractor = NewObject<USI_InventoryInteractor>(this);
	if (!InventoryInteractor) return;
	
	InventoryInteractor->Init(this, InteractionRadius);
}

int32 USI_InventoryComponent::RequestAddItem(FSI_AddItemRequest Request)
{
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = GenerateRequestId();
	}
	
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FSI_AddItemResponse Response;
		TryAddItemAuthority(Request, Response);
		OnAddItemRequestCompleted.Broadcast(Response);
		return Request.RequestId;
	}
	ServerRequestAddItem(Request);
	return Request.RequestId;
}

void USI_InventoryComponent::ServerRequestAddItem_Implementation(const FSI_AddItemRequest& Request)
{
	FSI_AddItemResponse Response;
	TryAddItemAuthority(Request, Response);
	ClientReceiveAddItemResponse(Response);
}

bool USI_InventoryComponent::TryAddItemAuthority(const FSI_AddItemRequest& Request, FSI_AddItemResponse& OutResponse)
{
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ContainerId = Request.ContainerId;
	OutResponse.ItemTag = Request.ItemTag;
	OutResponse.Result = ESI_AddItemResult::Failed;
	OutResponse.RequestedQuantity = Request.Quantity;
	OutResponse.AddedQuantity = 0;
	OutResponse.RemainingQuantity = Request.Quantity;
	
	if (GetOwner() && !GetOwner()->HasAuthority()) return false;
	
	if (!Request.ContainerId.IsValid()) return false;
	if (!Request.ItemTag.IsValid()) return false;
	if (Request.Quantity <= 0) return false;
	if (!IsItemAllowedInContainer(Request.ContainerId, Request.ItemTag)) return false;
	if (Request.ContainerId == GetEquipmentContainerId() && !DoesCharacterMeetItemLevelRequirement(Request.ItemTag)) return false;
	
	const int32 ContainerMaxSlots = GetContainerMaxSlots(Request.ContainerId);
	const int32 ItemMaxStackSize = GetItemMaxStackSize(Request.ItemTag);
	
	const FSI_AddItemResponse Response = InventoryMutator.AddItem(
		Inventory, Request, ContainerMaxSlots, ItemMaxStackSize, this);
	
	OutResponse = Response;
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ContainerId = Request.ContainerId;
	OutResponse.ItemTag = Request.ItemTag;
	
	// Notify the server to update the UI
	if (Response.SlotsToUpdate.Num() > 0)
	{
		BroadcastSlotsChanged(Request.ContainerId, Response.SlotsToUpdate);
	}
	
	// Main inventory quantity changed, so shortcut displays for this item may need a new count.
	if (Request.ContainerId == GetMainContainerId() && Response.AddedQuantity > 0)
	{
		RefreshReferencesForItem(Request.ItemTag);
	}

	if (Request.ContainerId == GetEquipmentContainerId() && Response.AddedQuantity > 0)
	{
		for (const int32 SlotIndex : Response.SlotsToUpdate)
		{
			UpdateItemEquippedStateForSlot(Request.ContainerId, SlotIndex);
		}

		RebuildEquipmentStatsForSlots(Response.SlotsToUpdate);
		RebuildEquipmentVisualsForSlots(Response.SlotsToUpdate);
	}
	
	return OutResponse.AddedQuantity > 0;
}

int32 USI_InventoryComponent::RequestRemoveItem(FSI_RemoveItemRequest Request)
{
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = GenerateRequestId();
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FSI_RemoveItemResponse Response;
		TryRemoveItemAuthority(Request, Response);
		OnRemoveItemRequestCompleted.Broadcast(Response);
		return Request.RequestId;
	}
	
	ServerRequestRemoveItem(Request);
	return Request.RequestId;
}

void USI_InventoryComponent::ServerRequestRemoveItem_Implementation(const FSI_RemoveItemRequest& Request)
{
	FSI_RemoveItemResponse Response;
	TryRemoveItemAuthority(Request, Response);
	ClientReceiveRemoveItemResponse(Response);
}

bool USI_InventoryComponent::TryRemoveItemAuthority(const FSI_RemoveItemRequest& Request, FSI_RemoveItemResponse& OutResponse)
{
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ContainerId = Request.ContainerId;
	OutResponse.SlotIndex = Request.SlotIndex;
	OutResponse.Result = ESI_RemoveItemResult::Failed;
	OutResponse.RequestedQuantity = Request.Quantity;
	OutResponse.RemovedQuantity = 0;
	OutResponse.RemainingQuantity = Request.Quantity;
	OutResponse.RemainingInSlot = 0;
	
	if (GetOwner() && !GetOwner()->HasAuthority()) return false;
	if (!Request.ContainerId.IsValid()) return false;
	if (Request.SlotIndex < 0) return false;
	if (Request.Quantity <= 0) return false;

	const FSI_InventoryEntry* EntryBeforeRemove = FindEntryAtSlot(Request.ContainerId, Request.SlotIndex);
	if (!EntryBeforeRemove) return false;

	// Only real main inventory items own the quantities that references display.
	const FGameplayTag RemovedItemTag = EntryBeforeRemove->ItemTag;
	OutResponse.ItemTag = RemovedItemTag;
	const bool bRemovedFromMainInventory =
		!EntryBeforeRemove->bIsReference && Request.ContainerId == GetMainContainerId();
	
	const FSI_RemoveItemResponse Response = InventoryMutator.RemoveItem(Inventory, Request);
	
	OutResponse = Response;
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ContainerId = Request.ContainerId;
	OutResponse.SlotIndex = Request.SlotIndex;
	OutResponse.ItemTag = RemovedItemTag;
	
	// Notify the server to update the UI
	if (Response.SlotsToUpdate.Num() > 0)
	{
		BroadcastSlotsChanged(Request.ContainerId, Response.SlotsToUpdate);
	}
	
	// Main inventory quantity changed, so shortcut displays may need refresh or cleanup.
	if (bRemovedFromMainInventory && Response.RemovedQuantity > 0)
	{
		RefreshReferencesForItem(RemovedItemTag);
	}

	if (Request.ContainerId == GetEquipmentContainerId() && Response.RemovedQuantity > 0)
	{
		for (const int32 SlotIndex : Response.SlotsToUpdate)
		{
			UpdateItemEquippedStateForSlot(Request.ContainerId, SlotIndex);
		}

		RebuildEquipmentStatsForSlots(Response.SlotsToUpdate);
		RebuildEquipmentVisualsForSlots(Response.SlotsToUpdate);
	}
	
	return OutResponse.RemovedQuantity > 0;
}

int32 USI_InventoryComponent::RequestRemoveItemFromContainer(FSI_RemoveItemFromContainerRequest Request)
{
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = GenerateRequestId();
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FSI_RemoveItemResponse Response;
		TryRemoveItemFromContainerAuthority(Request, Response);
		OnRemoveItemRequestCompleted.Broadcast(Response);
		return Request.RequestId;
	}
	
	ServerRequestRemoveItemFromContainer(Request);
	return Request.RequestId;
}

void USI_InventoryComponent::ServerRequestRemoveItemFromContainer_Implementation(const FSI_RemoveItemFromContainerRequest& Request)
{
	FSI_RemoveItemResponse Response;
	TryRemoveItemFromContainerAuthority(Request, Response);
	ClientReceiveRemoveItemResponse(Response);
}

bool USI_InventoryComponent::TryRemoveItemFromContainerAuthority(const FSI_RemoveItemFromContainerRequest& Request, FSI_RemoveItemResponse& OutResponse)
{
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ContainerId = Request.ContainerId;
	OutResponse.SlotIndex = INDEX_NONE;
	OutResponse.ItemTag = Request.ItemTag;
	OutResponse.Result = ESI_RemoveItemResult::Failed;
	OutResponse.RequestedQuantity = Request.bRemoveAll ? 0 : Request.Quantity;
	OutResponse.RemovedQuantity = 0;
	OutResponse.RemainingQuantity = Request.bRemoveAll ? 0 : Request.Quantity;
	OutResponse.RemainingInSlot = 0;
	OutResponse.SlotsToUpdate.Reset();
	
	if (GetOwner() && !GetOwner()->HasAuthority()) return false;
	if (!Request.ContainerId.IsValid()) return false;
	if (!Request.ItemTag.IsValid()) return false;
	if (!Request.bRemoveAll && Request.Quantity <= 0) return false;
	
	int32 TotalAvailable = 0;
	for (const FSI_InventoryEntry& Entry : Inventory.Entries)
	{
			if (Entry.ContainerId != Request.ContainerId) continue;
			if (Entry.ItemTag != Request.ItemTag) continue;
			if (Entry.SlotIndex < 0) continue;
			if (Entry.Quantity <= 0) continue;
			
			TotalAvailable += Entry.Quantity;
	}
	
	const int32 QuantityToRemove = Request.bRemoveAll ? TotalAvailable : Request.Quantity;
	OutResponse.RequestedQuantity = QuantityToRemove;
	OutResponse.RemainingQuantity = QuantityToRemove;
	
	if (QuantityToRemove <= 0) return false;
	
	int32 RemainingToRemove = QuantityToRemove;
	bool bRemovedOwnedMainInventoryItem = false;
	
	while (RemainingToRemove > 0)
	{
		const FSI_InventoryEntry* FirstMatchingEntry = nullptr;
		for (const FSI_InventoryEntry& Entry : Inventory.Entries)
		{
			if (Entry.ContainerId != Request.ContainerId) continue;
			if (Entry.ItemTag != Request.ItemTag) continue;
			if (Entry.SlotIndex < 0) continue;
			if (Entry.Quantity <= 0) continue;
			if (FirstMatchingEntry && Entry.SlotIndex >= FirstMatchingEntry->SlotIndex) continue;
			
			FirstMatchingEntry = &Entry;
		}
		
		if (!FirstMatchingEntry) break;
		
		const int32 SlotIndex = FirstMatchingEntry->SlotIndex;
		const bool bIsOwnedMainInventoryItem =
			!FirstMatchingEntry->bIsReference && Request.ContainerId == GetMainContainerId();
		if (OutResponse.SlotIndex == INDEX_NONE)
		{
			OutResponse.SlotIndex = SlotIndex;
		}
		
		FSI_RemoveItemRequest SlotRequest;
		SlotRequest.RequestId = Request.RequestId;
		SlotRequest.ContainerId = Request.ContainerId;
		SlotRequest.SlotIndex = SlotIndex;
		SlotRequest.Quantity = RemainingToRemove;
		
		const FSI_RemoveItemResponse SlotResponse = InventoryMutator.RemoveItem(Inventory, SlotRequest);
		if (SlotResponse.RemovedQuantity <= 0) break;
		
		OutResponse.RemovedQuantity += SlotResponse.RemovedQuantity;
		OutResponse.RemainingQuantity = FMath::Max(0, QuantityToRemove - OutResponse.RemovedQuantity);
		OutResponse.RemainingInSlot = SlotResponse.RemainingInSlot;
		RemainingToRemove = OutResponse.RemainingQuantity;
		bRemovedOwnedMainInventoryItem |= bIsOwnedMainInventoryItem;
		
		for (const int32 SlotToUpdate : SlotResponse.SlotsToUpdate)
		{
			OutResponse.SlotsToUpdate.AddUnique(SlotToUpdate);
		}
	}
	
	if (OutResponse.RemovedQuantity == 0)
	{
		OutResponse.Result = ESI_RemoveItemResult::Failed;
		return false;
	}
	
	OutResponse.Result = OutResponse.RemainingQuantity > 0
		? ESI_RemoveItemResult::PartialSuccess
		: ESI_RemoveItemResult::FullSuccess;
	
	if (OutResponse.SlotsToUpdate.Num() > 0)
	{
		BroadcastSlotsChanged(Request.ContainerId, OutResponse.SlotsToUpdate);
	}
	
	if (bRemovedOwnedMainInventoryItem)
	{
		RefreshReferencesForItem(Request.ItemTag);
	}

	if (Request.ContainerId == GetEquipmentContainerId())
	{
		for (const int32 SlotIndex : OutResponse.SlotsToUpdate)
		{
			UpdateItemEquippedStateForSlot(Request.ContainerId, SlotIndex);
		}

		RebuildEquipmentStatsForSlots(OutResponse.SlotsToUpdate);
		RebuildEquipmentVisualsForSlots(OutResponse.SlotsToUpdate);
	}
	
	return true;
}

int32 USI_InventoryComponent::RequestMoveItem(FSI_MoveItemRequest Request)
{
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = GenerateRequestId();
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FSI_MoveItemResponse Response;
		TryMoveItemAuthority(Request, Response);
		OnMoveItemRequestCompleted.Broadcast(Response);
		return Request.RequestId;
	}
	
	ServerRequestMoveItem(Request);
	return Request.RequestId;
}

void USI_InventoryComponent::ServerRequestMoveItem_Implementation(const FSI_MoveItemRequest& Request)
{
	FSI_MoveItemResponse Response;
	TryMoveItemAuthority(Request, Response);
	ClientReceiveMoveItemResponse(Response);
}

bool USI_InventoryComponent::TryMoveItemAuthority(const FSI_MoveItemRequest& Request, FSI_MoveItemResponse& OutResponse)
{
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ItemTag = FGameplayTag();
	OutResponse.Result = ESI_MoveItemResult::Failed;
	OutResponse.FailureReason = ESI_InventoryFailureReason::None;
	OutResponse.FromContainerId = Request.FromContainerId;
	OutResponse.FromSlotIndex = Request.FromSlotIndex;
	OutResponse.ToContainerId = Request.ToContainerId;
	OutResponse.ToSlotIndex = Request.ToSlotIndex;
	
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!Request.FromContainerId.IsValid()) return false;
	if (!Request.ToContainerId.IsValid()) return false;
	if (Request.FromSlotIndex < 0) return false;
	if (Request.ToSlotIndex < 0) return false;
	if (Request.FromContainerId == Request.ToContainerId && Request.FromSlotIndex == Request.ToSlotIndex) return false;
	
	const int32 FromContainerMaxSlots = GetContainerMaxSlots(Request.FromContainerId);
	const int32 ToContainerMaxSlots = GetContainerMaxSlots(Request.ToContainerId);
	if (FromContainerMaxSlots <= 0 || ToContainerMaxSlots <= 0) return false;
	if (Request.FromSlotIndex >= FromContainerMaxSlots || Request.ToSlotIndex >= ToContainerMaxSlots) return false;
	
	const FSI_InventoryEntry* FromEntry = FindEntryAtSlot(Request.FromContainerId, Request.FromSlotIndex);
	if (!FromEntry) return false;

	// References can be dragged too, but new references should point back to the real source item.
	FGameplayTag SourceContainerId = Request.FromContainerId;
	int32 SourceSlotIndex = Request.FromSlotIndex;
	FGameplayTag SourceItemTag = FromEntry->ItemTag;
	FSI_ItemInstanceData SourceItemInstanceData = FromEntry->ItemInstanceData;
	if (!SourceItemInstanceData.SourceItemTag.IsValid())
	{
		SourceItemInstanceData.SourceItemTag = SourceItemTag;
	}
	OutResponse.ItemTag = SourceItemTag;

	if (FromEntry->bIsReference)
	{
		if (!FromEntry->HasValidSourceReference()) return false;

		SourceContainerId = FromEntry->SourceContainerId;
		SourceSlotIndex = FromEntry->SourceSlotIndex;

		const FSI_InventoryEntry* SourceEntry = FindEntryAtSlot(SourceContainerId, SourceSlotIndex);
		if (!SourceEntry || SourceEntry->bIsReference) return false;

		SourceItemTag = SourceEntry->ItemTag;
		SourceItemInstanceData = SourceEntry->ItemInstanceData;
		if (!SourceItemInstanceData.SourceItemTag.IsValid())
		{
			SourceItemInstanceData.SourceItemTag = SourceItemTag;
		}
	}

	// Cross-container drops into shortcut containers create references instead of moving items.
	const FSI_ContainerConfig* ToConfig = GetContainerConfig(Request.ToContainerId);
	const bool bCrossContainerMove = Request.FromContainerId != Request.ToContainerId;
	const bool bShouldCreateReference = bCrossContainerMove && ToConfig &&
		ToConfig->bCreateReferenceOnCrossContainerDrop;

	if (bShouldCreateReference)
	{
		if (!IsItemAllowedInSlot(Request.ToContainerId, Request.ToSlotIndex, SourceItemTag))
		{
			return false;
		}
		
		// Skillbar references should only point at the player's main inventory.
		// This prevents storage/chest items from appearing usable on the skillbar.
		if (SourceContainerId != GetMainContainerId())
		{
			return false;
		}

		const FSI_MoveItemResponse Response = InventoryMutator.CreateReferenceEntry(
			Inventory, Request, SourceContainerId, SourceSlotIndex, SourceItemTag, SourceItemInstanceData);

		OutResponse = Response;
		OutResponse.RequestId = Request.RequestId;
		OutResponse.ItemTag = SourceItemTag;

		if (OutResponse.Result != ESI_MoveItemResult::Failed)
		{
			TMap<FGameplayTag, TArray<int32>> ChangedSlotsByContainer;

			// Refresh the dragged slot and the target shortcut slot.
			ChangedSlotsByContainer.FindOrAdd(Request.FromContainerId).AddUnique(Request.FromSlotIndex);
			ChangedSlotsByContainer.FindOrAdd(Request.ToContainerId).AddUnique(Request.ToSlotIndex);

			for (const TPair<FGameplayTag, TArray<int32>>& Pair : ChangedSlotsByContainer)
			{
				BroadcastSlotsChanged(Pair.Key, Pair.Value);
				ClientQueueSlotsChanged(Pair.Key, Pair.Value);
			}
		}

		return OutResponse.Result != ESI_MoveItemResult::Failed;
	}
	
	// References are shortcuts. They can move inside the same container,
	// or create another reference in a shortcut container, but they should not
	// be moved into a normal inventory container as if they were real items.
	if (FromEntry->bIsReference && bCrossContainerMove)
	{
		return false;
	}
	
	// The item being moved must be allowed in the exact destination slot.
	// This checks both the destination container rules and the destination slot profile.
	if (!IsItemAllowedInSlot(Request.ToContainerId, Request.ToSlotIndex, FromEntry->ItemTag))
	{
		return false;
	}
	
	const FSI_InventoryEntry* ToEntry = FindEntryAtSlot(Request.ToContainerId, Request.ToSlotIndex);
	if (ToEntry)
	{
		// References are shortcuts, so they should swap instead of merge.
		const bool bEitherEntryIsReference = FromEntry->bIsReference || ToEntry->bIsReference;
		const bool bWouldMerge =
			!bEitherEntryIsReference &&
			FromEntry->ItemTag == ToEntry->ItemTag &&
			ToEntry->Quantity < GetItemMaxStackSize(FromEntry->ItemTag);

		if (!bWouldMerge)
		{
			// If this move becomes a swap, the destination item must be allowed
			// back in the original source slot.
			if (!IsItemAllowedInSlot(Request.FromContainerId, Request.FromSlotIndex, ToEntry->ItemTag))
			{
				return false;
			}
		}
	}
	
	const int32 ItemMaxStackSize = GetItemMaxStackSize(FromEntry->ItemTag);
	if (ItemMaxStackSize <= 0) return false;
	
	const FGameplayTag MainContainerId = GetMainContainerId();
	const FGameplayTag EquipmentContainerId = GetEquipmentContainerId();
	if (EquipmentContainerId.IsValid())
	{
		if (Request.ToContainerId == EquipmentContainerId && !DoesCharacterMeetItemLevelRequirement(FromEntry->ItemTag))
		{
			OutResponse.FailureReason = ESI_InventoryFailureReason::LevelRequirementNotMet;
			return false;
		}

		if (ToEntry && Request.FromContainerId == EquipmentContainerId && !DoesCharacterMeetItemLevelRequirement(ToEntry->ItemTag))
		{
			OutResponse.FailureReason = ESI_InventoryFailureReason::LevelRequirementNotMet;
			return false;
		}
	}

	TArray<int32> EquipmentSlotsNeedingStatRebuild;

	if (EquipmentContainerId.IsValid())
	{
		if (Request.FromContainerId == EquipmentContainerId)
		{
			EquipmentSlotsNeedingStatRebuild.AddUnique(Request.FromSlotIndex);
		}

		if (Request.ToContainerId == EquipmentContainerId)
		{
			EquipmentSlotsNeedingStatRebuild.AddUnique(Request.ToSlotIndex);
		}
	}

	// Remember which item counts may change because of this move.
	TArray<FGameplayTag> ItemsNeedingReferenceRefresh;

	if (FromEntry && !FromEntry->bIsReference && Request.FromContainerId == MainContainerId)
	{
		ItemsNeedingReferenceRefresh.AddUnique(FromEntry->ItemTag);
	}

	if (ToEntry && !ToEntry->bIsReference && Request.ToContainerId == MainContainerId)
	{
		ItemsNeedingReferenceRefresh.AddUnique(ToEntry->ItemTag);
	}
	
	const FSI_MoveItemResponse Response = InventoryMutator.MoveItem(
		Inventory, Request.FromContainerId, Request.FromSlotIndex, Request.ToContainerId, Request.ToSlotIndex, ItemMaxStackSize);
	
	OutResponse = Response;
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ItemTag = FromEntry->ItemTag;
	
	if (OutResponse.Result != ESI_MoveItemResult::Failed)
	{
		TMap<FGameplayTag, TArray<int32>> ChangeSlotsByContainer;
		
		ChangeSlotsByContainer.FindOrAdd(Request.FromContainerId).AddUnique(Request.FromSlotIndex);
		ChangeSlotsByContainer.FindOrAdd(Request.ToContainerId).AddUnique(Request.ToSlotIndex);
		
		for (const TPair<FGameplayTag, TArray<int32>>& Pair : ChangeSlotsByContainer)
		{
			for (const int32 SlotIndex : Pair.Value)
			{
				UpdateItemEquippedStateForSlot(Pair.Key, SlotIndex);
			}
		}

		for (const TPair<FGameplayTag, TArray<int32>>& Pair : ChangeSlotsByContainer)
		{
			BroadcastSlotsChanged(Pair.Key, Pair.Value);
			ClientQueueSlotsChanged(Pair.Key, Pair.Value);
		}
		
		// Moving items in or out of main inventory changes shortcut counts.
		for (const FGameplayTag& ItemTag : ItemsNeedingReferenceRefresh)
		{
			RefreshReferencesForItem(ItemTag);
		}

		RebuildEquipmentStatsForSlots(EquipmentSlotsNeedingStatRebuild);
		RebuildEquipmentVisualsForSlots(EquipmentSlotsNeedingStatRebuild);
	}
	
	return OutResponse.Result != ESI_MoveItemResult::Failed;
}

int32 USI_InventoryComponent::RequestSplitItem(FSI_SplitStackRequest Request)
{
	if (Request.RequestId == INDEX_NONE)
	{
		Request.RequestId = GenerateRequestId();
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FSI_SplitStackResponse Response;
		TrySplitItemAuthority(Request, Response);
		OnSplitItemRequestCompleted.Broadcast(Response);
		return Request.RequestId;
	}
	
	ServerRequestSplitItem(Request);
	return Request.RequestId;
}

void USI_InventoryComponent::ServerRequestSplitItem_Implementation(const FSI_SplitStackRequest& Request)
{
	FSI_SplitStackResponse Response;
	TrySplitItemAuthority(Request, Response);
	ClientReceiveSplitItemResponse(Response);
}

bool USI_InventoryComponent::TrySplitItemAuthority(const FSI_SplitStackRequest& Request, FSI_SplitStackResponse& OutResponse)
{
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ItemTag = FGameplayTag();
	OutResponse.Result = ESI_SplitStackResult::Failed;
	OutResponse.ContainerId = Request.ContainerId;
	OutResponse.SlotIndex = Request.SlotIndex;
	OutResponse.NewSlotIndex = INDEX_NONE;
	OutResponse.SplitQuantity = 0;
	
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!Request.ContainerId.IsValid()) return false;
	if (Request.SlotIndex < 0) return false;
	if (Request.SplitQuantity <= 0) return false;
	
	const int32 MaxSlots = GetContainerMaxSlots(Request.ContainerId);
	if (MaxSlots <= 0) return false;
	if (Request.SlotIndex >= MaxSlots) return false;
	
	const FSI_InventoryEntry* SourceEntry = FindEntryAtSlot(Request.ContainerId, Request.SlotIndex);
	if (!SourceEntry) return false;
	OutResponse.ItemTag = SourceEntry->ItemTag;
	
	const USI_ItemDatabase* Database = GetItemDatabase();
	if (!Database) return false;
	
	const USI_ItemDefinition* Def = Database->GetItemDefinitionByTag(SourceEntry->ItemTag);
	if (!Def || !Def->bStackable) return false;
	
	const FSI_SplitStackResponse Response = InventoryMutator.SplitStack(
		Inventory, Request.ContainerId, Request.SlotIndex, Request.SplitQuantity, MaxSlots, this);
	
	OutResponse = Response;
	OutResponse.RequestId = Request.RequestId;
	OutResponse.ItemTag = SourceEntry->ItemTag;
	
	if (OutResponse.SlotsToUpdate.Num() > 0)
	{
		BroadcastSlotsChanged(Request.ContainerId, Response.SlotsToUpdate);
	}
	
	return OutResponse.Result == ESI_SplitStackResult::Success;
}

void USI_InventoryComponent::RequestPickupItem()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		TryPickupItemAuthority();
		return;
	}
	
	ServerRequestPickupItem();
}

void USI_InventoryComponent::ServerRequestPickupItem_Implementation()
{
	TryPickupItemAuthority();
}

void USI_InventoryComponent::TryPickupItemAuthority()
{
	if (!InventoryInteractor) return;
	InventoryInteractor->PickupItem();
}

void USI_InventoryComponent::RequestUseItem(const FGameplayTag& ContainerId, int32 SlotIndex, bool bPressed)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		TryUseItemAuthority(ContainerId, SlotIndex, bPressed);
		return;
	}
	
	ServerRequestUseItem(ContainerId, SlotIndex, bPressed);
}

void USI_InventoryComponent::ServerRequestUseItem_Implementation(const FGameplayTag& ContainerId,
	int32 SlotIndex, bool bPressed)
{
	TryUseItemAuthority(ContainerId, SlotIndex, bPressed);
}

bool USI_InventoryComponent::TryUseItemAuthority(const FGameplayTag& ContainerId, int32 SlotIndex, bool bPressed)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!ContainerId.IsValid()) return false;
	if (SlotIndex < 0) return false;
	if (!ItemDatabase) return false;
	
	const FSI_InventoryEntry* Entry = FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry) return false;
	
	// References are shortcuts. Using one should consume the real main inventory item.
	FGameplayTag UseContainerId = ContainerId;
	int32 UseSlotIndex = SlotIndex;
	FGameplayTag UsedItemTag = Entry->ItemTag;

	if (Entry->bIsReference)
	{
		if (!Entry->HasValidSourceReference()) return false;

		UseContainerId = Entry->SourceContainerId;
		UseSlotIndex = Entry->SourceSlotIndex;

		const FGameplayTag MainContainerId = GetMainContainerId();
		if (!MainContainerId.IsValid() || UseContainerId != MainContainerId)
		{
			return false;
		}

		const FSI_InventoryEntry* SourceEntry = FindEntryAtSlot(UseContainerId, UseSlotIndex);

		// If the original stack moved or merged, find any main inventory stack for the same item.
		if (!SourceEntry || SourceEntry->bIsReference || SourceEntry->ItemTag != Entry->ItemTag)
		{
			SourceEntry = nullptr;

			for (const FSI_InventoryEntry& CandidateEntry : Inventory.Entries)
			{
				if (CandidateEntry.bIsReference) continue;
				if (CandidateEntry.ContainerId != MainContainerId) continue;
				if (CandidateEntry.ItemTag != Entry->ItemTag) continue;
				if (CandidateEntry.Quantity <= 0) continue;

				UseSlotIndex = CandidateEntry.SlotIndex;
				SourceEntry = &CandidateEntry;
				break;
			}
		}

		if (!SourceEntry) return false;

		UsedItemTag = SourceEntry->ItemTag;
	}
	
	const USI_ItemDefinition* Def = ItemDatabase->GetItemDefinitionByTag(UsedItemTag);
	if (!Def) return false;
	if (!DoesCharacterMeetItemLevelRequirement(UsedItemTag)) return false;
	
	if (!Def->bCanUse) return false;
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;
	
	if (bPressed)
	{
		for (const TSubclassOf<UGameplayEffect>& EffectClass : Def->UseGameplayEffects)
		{
			const UGameplayEffect* EffectCDO = EffectClass.GetDefaultObject();
			if (!EffectCDO) return false;
		
			const FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectToSelf(EffectCDO, 1.f, ASC->MakeEffectContext());
		
			const bool bIsInstantEffect = EffectCDO->DurationPolicy == EGameplayEffectDurationType::Instant;
		
			/* For duration/infinite effects, that handle should be valid because the effect is added to the active effect container.
			For instant effects, there often is no lasting active effect entry. */
			if (!bIsInstantEffect && !EffectHandle.IsValid()) return false;
		}

		if (Def->bApplyCooldownOnUse && Def->CooldownGameplayEffect)
		{
			const UGameplayEffect* CooldownEffectCDO = Def->CooldownGameplayEffect.GetDefaultObject();
			if (!CooldownEffectCDO) return false;

			const FActiveGameplayEffectHandle CooldownEffectHandle =
				ASC->ApplyGameplayEffectToSelf(CooldownEffectCDO, 1.f, ASC->MakeEffectContext());

			const bool bIsInstantCooldownEffect =
				CooldownEffectCDO->DurationPolicy == EGameplayEffectDurationType::Instant;

			if (!bIsInstantCooldownEffect && !CooldownEffectHandle.IsValid()) return false;
		}
	}
	
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Def->GameplayAbilities)
	{
		if (!IsValid(AbilityClass)) continue;

		FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(AbilityClass);
		if (!Spec) return false;
		
		if (bPressed)
		{
			ASC->AbilitySpecInputPressed(*Spec);
			
			FPredictionKey PredictionKey;
			if (UGameplayAbility* PrimaryInstance = Spec->GetPrimaryInstance())
			{
				PredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
			}

			ASC->InvokeReplicatedEvent(
				EAbilityGenericReplicatedEvent::InputPressed, Spec->Handle, PredictionKey);
			
			if (!Spec->IsActive() && !ASC->TryActivateAbility(Spec->Handle))
			{
				return false;
			}
		}
		else
		{
			ASC->AbilitySpecInputReleased(*Spec);
			
			if (Spec->IsActive())
			{
				FPredictionKey PredictionKey;
				if (UGameplayAbility* PrimaryInstance = Spec->GetPrimaryInstance())
				{
					PredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
				}

				ASC->InvokeReplicatedEvent(
					EAbilityGenericReplicatedEvent::InputReleased, Spec->Handle, PredictionKey);
			}
		}
	}
	
	if (!bPressed || !Def->bConsumeOnUse) return true;
	
	FSI_RemoveItemRequest Request;
	Request.ContainerId = UseContainerId;
	Request.SlotIndex = UseSlotIndex;
	Request.Quantity = Def->AmountToConsume;
	
	FSI_RemoveItemResponse Response;
	const bool bRemoved = TryRemoveItemAuthority(Request, Response);
	
	return bRemoved;
}

void USI_InventoryComponent::RebuildEquipmentStatsForSlots(const TArray<int32>& SlotIndices)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (SlotIndices.IsEmpty()) return;

	for (const int32 SlotIndex : SlotIndices)
	{
		if (SlotIndex < 0) continue;

		RemoveEquipmentStatsFromSlot(SlotIndex);
		ApplyEquipmentStatsFromSlot(SlotIndex);
	}
}

void USI_InventoryComponent::RemoveEquipmentStatsFromSlot(int32 SlotIndex)
{
	if (SlotIndex < 0) return;

	if (FActiveGameplayEffectHandle* EffectHandle = ActiveEquipmentStatEffects.Find(SlotIndex))
	{
		if (EffectHandle->IsValid())
		{
			if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				ASC->RemoveActiveGameplayEffect(*EffectHandle);
			}
		}
	}

	ActiveEquipmentStatEffects.Remove(SlotIndex);
	ActiveEquipmentStatEffectDefinitions.Remove(SlotIndex);
}

void USI_InventoryComponent::ApplyEquipmentStatsFromSlot(int32 SlotIndex)
{
	if (SlotIndex < 0) return;
	if (!ItemDatabase) return;
	if (!ItemStatRegistry) return;

	const FGameplayTag EquipmentContainerId = GetEquipmentContainerId();
	if (!EquipmentContainerId.IsValid()) return;

	const FSI_InventoryEntry* Entry = FindEntryAtSlot(EquipmentContainerId, SlotIndex);
	if (!Entry || Entry->bIsReference) return;

	const USI_ItemDefinition* ItemDefinition = ItemDatabase->GetItemDefinitionByTag(Entry->ItemTag);
	if (!ItemDefinition || !ItemDefinition->bEquippable || ItemDefinition->EquipStats.IsEmpty()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	UGameplayEffect* RuntimeEffect = NewObject<UGameplayEffect>(
		this,
		MakeUniqueObjectName(this, UGameplayEffect::StaticClass(), TEXT("GE_EquipmentItemStats")));
	if (!RuntimeEffect) return;

	RuntimeEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;

	for (const FSI_ItemStatModifier& ItemStat : ItemDefinition->EquipStats)
	{
		FGameplayAttribute Attribute;
		if (!ItemStatRegistry->FindAttributeForStatTag(ItemStat.StatTag, Attribute))
		{
			UE_LOG(LogTemp, Warning, TEXT("No attribute mapping found for item stat tag %s on item %s."),
				*ItemStat.StatTag.ToString(),
				*Entry->ItemTag.ToString());
			continue;
		}

		FGameplayModifierInfo& Modifier = RuntimeEffect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = ItemStat.Operation;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ItemStat.Magnitude));
	}

	if (RuntimeEffect->Modifiers.IsEmpty()) return;

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(ItemDefinition);

	const FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectToSelf(RuntimeEffect, 1.f, EffectContext);
	if (!EffectHandle.IsValid()) return;

	ActiveEquipmentStatEffects.Add(SlotIndex, EffectHandle);
	ActiveEquipmentStatEffectDefinitions.Add(SlotIndex, RuntimeEffect);
}

void USI_InventoryComponent::RebuildEquipmentVisualsForSlots(const TArray<int32>& SlotIndices)
{
	if (SlotIndices.IsEmpty()) return;

	for (const int32 SlotIndex : SlotIndices)
	{
		if (SlotIndex < 0) continue;

		RemoveEquipmentVisualFromSlot(SlotIndex);
		ApplyEquipmentVisualFromSlot(SlotIndex);
	}
}

void USI_InventoryComponent::RebuildAllEquipmentVisuals()
{
	const FGameplayTag EquipmentContainerId = GetEquipmentContainerId();
	if (!EquipmentContainerId.IsValid()) return;

	TArray<int32> SlotIndices;

	for (const TPair<int32, TObjectPtr<UStaticMeshComponent>>& Pair : ActiveEquipmentMeshes)
	{
		SlotIndices.AddUnique(Pair.Key);
	}

	for (const FSI_InventoryEntry& Entry : Inventory.Entries)
	{
		if (!Entry.IsValidEntry()) continue;
		if (Entry.ContainerId != EquipmentContainerId) continue;

		SlotIndices.AddUnique(Entry.SlotIndex);
	}

	RebuildEquipmentVisualsForSlots(SlotIndices);
}

void USI_InventoryComponent::RemoveEquipmentVisualFromSlot(int32 SlotIndex)
{
	if (SlotIndex < 0) return;

	if (TObjectPtr<UStaticMeshComponent>* MeshComponent = ActiveEquipmentMeshes.Find(SlotIndex))
	{
		if (IsValid(MeshComponent->Get()))
		{
			MeshComponent->Get()->DestroyComponent();
		}
	}

	ActiveEquipmentMeshes.Remove(SlotIndex);
}

void USI_InventoryComponent::ApplyEquipmentVisualFromSlot(int32 SlotIndex)
{
	if (SlotIndex < 0) return;
	if (!ItemDatabase) return;

	const FGameplayTag EquipmentContainerId = GetEquipmentContainerId();
	if (!EquipmentContainerId.IsValid()) return;

	const FSI_InventoryEntry* Entry = FindEntryAtSlot(EquipmentContainerId, SlotIndex);
	if (!Entry || Entry->bIsReference) return;

	const USI_ItemDefinition* ItemDefinition = ItemDatabase->GetItemDefinitionByTag(Entry->ItemTag);
	if (!ItemDefinition || !ItemDefinition->bEquippable || !ItemDefinition->MeshToSpawn) return;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	AActor* MeshOwnerActor = OwnerActor;

	if (const AController* OwnerController = Cast<AController>(OwnerActor))
	{
		MeshOwnerActor = OwnerController->GetPawn<APawn>();
	}

	if (!MeshOwnerActor) return;

	USkeletalMeshComponent* CharacterMesh = MeshOwnerActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!CharacterMesh) return;

	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(MeshOwnerActor);
	if (!MeshComponent) return;

	MeshComponent->SetStaticMesh(ItemDefinition->MeshToSpawn);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->RegisterComponent();

	MeshComponent->AttachToComponent(
		CharacterMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		ItemDefinition->SocketName
	);

	MeshComponent->SetRelativeLocation(ItemDefinition->RelativeLocation);
	MeshComponent->SetRelativeRotation(ItemDefinition->RelativeRotation);
	MeshComponent->SetRelativeScale3D(ItemDefinition->RelativeScale);

	ActiveEquipmentMeshes.Add(SlotIndex, MeshComponent);
}

void USI_InventoryComponent::HandleReplicationAdd(const TArrayView<int32>& Indices)
{
	TMap<FGameplayTag, TArray<int32>> ChangedSlotsByContainer;
	
	for (int32 Index : Indices)
	{
		if (!Inventory.Entries.IsValidIndex(Index)) continue;
		
		const FSI_InventoryEntry& Entry = Inventory.Entries[Index];
		
		if (!Entry.IsValidEntry()) continue;
		
		ChangedSlotsByContainer.FindOrAdd(Entry.ContainerId).AddUnique(Entry.SlotIndex);
	}
	
	for (const TPair<FGameplayTag, TArray<int32>>& Pair : ChangedSlotsByContainer)
	{
		BroadcastSlotsChanged(Pair.Key, Pair.Value);
	}

	RebuildAllEquipmentVisuals();
}

void USI_InventoryComponent::HandleReplicationRemove(const TArray<FSI_InventoryEntry>& RemovedEntries)
{
	TMap<FGameplayTag, TArray<int32>> ChangedSlotsByContainer;
	
	for (const FSI_InventoryEntry& Entry : RemovedEntries)
	{
		if (!Entry.IsValidEntry()) continue;
		
		ChangedSlotsByContainer.FindOrAdd(Entry.ContainerId).AddUnique(Entry.SlotIndex);
	}
	
	for (const TPair<FGameplayTag, TArray<int32>>& Pair : ChangedSlotsByContainer)
	{
		BroadcastSlotsChanged(Pair.Key, Pair.Value);
	}

	RebuildAllEquipmentVisuals();
}

void USI_InventoryComponent::HandleReplicationChange(const TArrayView<int32>& Indices)
{
	TMap<FGameplayTag, TArray<int32>> ChangedSlotsByContainer;
	
	for (int32 Index : Indices)
	{
		if (!Inventory.Entries.IsValidIndex(Index)) continue;
		
		const FSI_InventoryEntry& Entry = Inventory.Entries[Index];
		
		if (!Entry.IsValidEntry()) continue;
		
		ChangedSlotsByContainer.FindOrAdd(Entry.ContainerId).AddUnique(Entry.SlotIndex);
	}
	
	for (const TPair<FGameplayTag, TArray<int32>>& Pair : ChangedSlotsByContainer)
	{
		BroadcastSlotsChanged(Pair.Key, Pair.Value);
	}

	RebuildAllEquipmentVisuals();
}

void USI_InventoryComponent::BroadcastSlotsChanged(const FGameplayTag& ContainerTag, const TArray<int32>& SlotIndices)
{
	if (!ContainerTag.IsValid() || SlotIndices.Num() == 0) return;
	
	OnSlotChanged.Broadcast(ContainerTag, SlotIndices);
}

void USI_InventoryComponent::ClientQueueSlotsChanged_Implementation(const FGameplayTag& ContainerId,
                                                                   const TArray<int32>& SlotIndices)
{
	if (!ContainerId.IsValid() || SlotIndices.Num() == 0) return;
	
	TArray<int32>& PendingSlots = PendingClientSlotRefreshes.FindOrAdd(ContainerId);
	for (const int32 SlotIndex : SlotIndices)
	{
		PendingSlots.AddUnique(SlotIndex);
	}
}

void USI_InventoryComponent::FlushPendingClientSlotRefreshes()
{
	if (PendingClientSlotRefreshes.IsEmpty()) return;
	
	for (TPair<FGameplayTag, TArray<int32>>& Pair : PendingClientSlotRefreshes)
	{
		if (!Pair.Key.IsValid() || Pair.Value.Num() == 0) continue;
		
		OnSlotChanged.Broadcast(Pair.Key, Pair.Value);
	}
	
	PendingClientSlotRefreshes.Empty();
}

void USI_InventoryComponent::ClientReceiveAddItemResponse_Implementation(const FSI_AddItemResponse& Response)
{
	OnAddItemRequestCompleted.Broadcast(Response);
}

void USI_InventoryComponent::ClientReceiveRemoveItemResponse_Implementation(const FSI_RemoveItemResponse& Response)
{
	OnRemoveItemRequestCompleted.Broadcast(Response);
}

void USI_InventoryComponent::ClientReceiveMoveItemResponse_Implementation(const FSI_MoveItemResponse& Response)
{
	OnMoveItemRequestCompleted.Broadcast(Response);
}

void USI_InventoryComponent::ClientReceiveSplitItemResponse_Implementation(const FSI_SplitStackResponse& Response)
{
	OnSplitItemRequestCompleted.Broadcast(Response);
}

int32 USI_InventoryComponent::GetContainerMaxSlots(const FGameplayTag& ContainerId) const
{
	for (const FSI_ContainerConfig Config : ContainerConfigs)
	{
		if (Config.ContainerId == ContainerId)
		{
			return Config.MaxSlots;
		}
	}
	
	return 0;
}

int32 USI_InventoryComponent::GetItemMaxStackSize(const FGameplayTag& ItemTag) const
{
	if (!ItemDatabase) return 0;
	
	const USI_ItemDefinition* Def = ItemDatabase->GetItemDefinitionByTag(ItemTag);
	if (!Def) return 0;
	
	return Def->bStackable ? FMath::Max(1, Def->MaxStackSize) : 1;
}

const FSI_ContainerConfig* USI_InventoryComponent::GetContainerConfig(const FGameplayTag& ContainerId) const
{
	for (const FSI_ContainerConfig& Config : ContainerConfigs)
	{
		if (Config.ContainerId == ContainerId)
		{
			return &Config;
		}
	}
	
	return nullptr;
}

const FSI_InventoryEntry* USI_InventoryComponent::FindEntryAtSlot(const FGameplayTag& ContainerId, int32 SlotIndex) const
{
	if (!ContainerId.IsValid() || SlotIndex < 0) return nullptr;
	
	for (const FSI_InventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.ContainerId == ContainerId && Entry.SlotIndex == SlotIndex)
		{
			return &Entry;
		}
	}
	
	return nullptr;
}

bool USI_InventoryComponent::GetItemInstanceDataAtSlot(const FGameplayTag& ContainerId, int32 SlotIndex,
	FSI_ItemInstanceData& OutItemInstanceData) const
{
	OutItemInstanceData = FSI_ItemInstanceData();

	const FSI_InventoryEntry* Entry = FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry) return false;

	OutItemInstanceData = Entry->ItemInstanceData;
	if (!OutItemInstanceData.SourceItemTag.IsValid())
	{
		OutItemInstanceData.SourceItemTag = Entry->ItemTag;
	}

	return true;
}

int32 USI_InventoryComponent::GetMainInventoryQuantityForItem(const FGameplayTag& ItemTag) const
{
	if (!ItemTag.IsValid()) return 0;

	const FGameplayTag MainContainerId = GetMainContainerId();
	if (!MainContainerId.IsValid()) return 0;

	int32 TotalQuantity = 0;

	for (const FSI_InventoryEntry& Entry : Inventory.Entries)
	{
		// References and storage entries do not count as usable player inventory.
		if (Entry.bIsReference) continue;
		if (Entry.ContainerId != MainContainerId) continue;
		if (Entry.ItemTag != ItemTag) continue;

		TotalQuantity += Entry.Quantity;
	}

	return TotalQuantity;
}

FGameplayTag USI_InventoryComponent::GetMainContainerId() const
{
	for (const FSI_ContainerConfig& Config : ContainerConfigs)
	{
		if (Config.bIsMainInventory && Config.ContainerId.IsValid())
		{
			return Config.ContainerId;
		}
	}
	
	return FGameplayTag();
}

FGameplayTag USI_InventoryComponent::GetEquipmentContainerId() const
{
	for (const FSI_ContainerConfig& Config : ContainerConfigs)
	{
		if (Config.bIsEquipmentInventory && Config.ContainerId.IsValid())
		{
			return Config.ContainerId;
		}
	}
	
	return FGameplayTag();
}

const USI_SlotLayoutConfig* USI_InventoryComponent::GetSlotLayoutConfig(const FGameplayTag& ContainerId) const
{
	// Reuse the existing container lookup so we keep one source of truth.
	const FSI_ContainerConfig* Config = GetContainerConfig(ContainerId);
	
	if (!Config) return nullptr;
	
	return Config->SlotLayoutConfig.Get();
}

const USI_SlotProfile* USI_InventoryComponent::GetSlotProfileForSlot(const FGameplayTag& ContainerId,
	int32 SlotIndex) const
{
	if (SlotIndex < 0) return nullptr;
	
	const USI_SlotLayoutConfig* LayoutConfig = GetSlotLayoutConfig(ContainerId);
	if (!LayoutConfig) return nullptr;

	if (!LayoutConfig->SlotProfileRegistry) return nullptr;

	// Find the layout entry for this exact slot.
	const FSI_SlotLayoutEntry* SlotEntry = LayoutConfig->FindSlotEntryByIndex(SlotIndex);
	if (!SlotEntry) return nullptr;

	// No profile id means this slot intentionally has no profile.
	if (SlotEntry->SlotProfileId.IsNone()) return nullptr;

	// Return the actual profile from the registry.
	return LayoutConfig->SlotProfileRegistry->FindProfileById(SlotEntry->SlotProfileId);
}

const FSI_SlotLayoutEntry* USI_InventoryComponent::GetSlotLayoutEntry(const FGameplayTag& ContainerId,
	int32 SlotIndex) const
{
	if (SlotIndex < 0) return nullptr;
	
	const USI_SlotLayoutConfig* LayoutConfig = GetSlotLayoutConfig(ContainerId);
	if (!LayoutConfig) return nullptr;

	return LayoutConfig->FindSlotEntryByIndex(SlotIndex);
}

UAbilitySystemComponent* USI_InventoryComponent::GetAbilitySystemComponent()
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent.Get();
	}
	
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;
	
	APlayerController* PC = Cast<APlayerController>(OwnerActor);
	if (!PC) return nullptr;
	
	if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS))
		{
			AbilitySystemComponent = ASC;
			return ASC;
		}
	}
	
	if (APawn* Pawn = PC->GetPawn<APawn>())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			AbilitySystemComponent = ASC;
			return ASC;
		}
	}
	
	return nullptr;
}

void USI_InventoryComponent::SetCharacterLevel(int32 NewCharacterLevel)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerSetCharacterLevel(NewCharacterLevel);
		return;
	}

	CharacterLevel = FMath::Max(1, NewCharacterLevel);
}

void USI_InventoryComponent::ServerSetCharacterLevel_Implementation(int32 NewCharacterLevel)
{
	SetCharacterLevel(NewCharacterLevel);
}

bool USI_InventoryComponent::DoesCharacterMeetItemLevelRequirement(const FGameplayTag& ItemTag) const
{
	if (!ItemTag.IsValid()) return false;
	if (!ItemDatabase) return false;

	const USI_ItemDefinition* Def = ItemDatabase->GetItemDefinitionByTag(ItemTag);
	if (!Def) return false;

	return CharacterLevel >= Def->RequiredLevel;
}

bool USI_InventoryComponent::IsItemAllowedInContainer(const FGameplayTag& ContainerId,
	const FGameplayTag& ItemTag) const
{
	if (!ContainerId.IsValid() || !ItemTag.IsValid()) return false;
	
	const FSI_ContainerConfig* Config = GetContainerConfig(ContainerId);
	if (!Config) return false;
	
	if (Config->AllowedItemTypes.IsEmpty() && Config->AllowedEquipmentSlots.IsEmpty()) return true;
	
	if (!ItemDatabase) return false;
	const USI_ItemDefinition* Def = ItemDatabase->GetItemDefinitionByTag(ItemTag);
	if (!Def) return false;
	
	if (!Config->AllowedItemTypes.IsEmpty())
	{
		bool bMatchesAllowedType = false;

		for (const FGameplayTag& AllowedItemType : Config->AllowedItemTypes)
		{
			if (AllowedItemType.IsValid() && Def->ItemType.MatchesTag(AllowedItemType))
			{
				bMatchesAllowedType = true;
				break;
			}
		}

		if (!bMatchesAllowedType) return false;
	}

	if (!Config->AllowedEquipmentSlots.IsEmpty())
	{
		bool bMatchesAllowedEquipmentSlot = false;

		for (const FGameplayTag& AllowedEquipmentSlot : Config->AllowedEquipmentSlots)
		{
			if (AllowedEquipmentSlot.IsValid() && Def->EquipSlot.MatchesTag(AllowedEquipmentSlot))
			{
				bMatchesAllowedEquipmentSlot = true;
				break;
			}
		}

		if (!bMatchesAllowedEquipmentSlot) return false;
	}

	return true;
}

bool USI_InventoryComponent::IsItemAllowedInSlot(const FGameplayTag& ContainerId, int32 SlotIndex,
	const FGameplayTag& ItemTag) const
{
	if (SlotIndex < 0) return false;
	if (!ContainerId.IsValid()) return false;
	if (!ItemTag.IsValid()) return false;
	if (!IsItemAllowedInContainer(ContainerId, ItemTag)) return false;
	if (!ItemDatabase) return false;
	
	const USI_ItemDefinition* ItemDefinition = ItemDatabase->GetItemDefinitionByTag(ItemTag);
	if (!ItemDefinition) return false;
	
	const USI_SlotLayoutConfig* LayoutConfig = GetSlotLayoutConfig(ContainerId);
	if (!LayoutConfig) return true;

	// Find the layout entry for this exact slot index.
	const FSI_SlotLayoutEntry* SlotEntry = LayoutConfig->FindSlotEntryByIndex(SlotIndex);

	// No slot entry means this slot has no special rule.
	if (!SlotEntry) return true;

	// No profile id means this slot has no special rule.
	if (SlotEntry->SlotProfileId.IsNone()) return true;

	// The layout needs a registry so it can resolve the profile id.
	if (!LayoutConfig->SlotProfileRegistry) return true;

	// Find the actual slot profile from the registry.
	const USI_SlotProfile* SlotProfile = LayoutConfig->SlotProfileRegistry->FindProfileById(SlotEntry->SlotProfileId);

	// Missing profile is treated as no special rule for now.
	// The data validation step already warns about missing profiles.
	if (!SlotProfile) return true;

	// Finally check the item against the profile rules.
	return DoesItemMatchSlotProfile(ItemDefinition, SlotProfile);
}

int32 USI_InventoryComponent::FindFirstFreeAllowedSlot(const FGameplayTag& ContainerId,
	const FGameplayTag& ItemTag) const
{
	if (!ContainerId.IsValid()) return INDEX_NONE;
	if (!ItemTag.IsValid()) return INDEX_NONE;
	
	const int32 MaxSlots = GetContainerMaxSlots(ContainerId);
	if (MaxSlots <= 0) return INDEX_NONE;

	// Check each slot from first to last.
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		// If something is already in this slot, it is not free.
		if (FindEntryAtSlot(ContainerId, SlotIndex)) continue;

		// If the item is not allowed in this exact slot, skip it.
		if (!IsItemAllowedInSlot(ContainerId, SlotIndex, ItemTag)) continue;

		// This slot is empty and accepts the item.
		return SlotIndex;
	}

	// No empty allowed slot was found.
	return INDEX_NONE;
}

int32 USI_InventoryComponent::FindBestEquipmentSlotForItem(const FGameplayTag& ItemTag) const
{
	if (!ItemTag.IsValid()) return INDEX_NONE;
	if (!ItemDatabase) return INDEX_NONE;

	const USI_ItemDefinition* ItemDefinition = ItemDatabase->GetItemDefinitionByTag(ItemTag);
	if (!ItemDefinition || !ItemDefinition->bEquippable || !ItemDefinition->EquipSlot.IsValid())
	{
		return INDEX_NONE;
	}

	const FGameplayTag EquipmentContainerId = GetEquipmentContainerId();
	if (!EquipmentContainerId.IsValid()) return INDEX_NONE;

	const int32 MaxSlots = GetContainerMaxSlots(EquipmentContainerId);
	if (MaxSlots <= 0) return INDEX_NONE;

	// Prefer an empty matching equipment slot.
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		if (FindEntryAtSlot(EquipmentContainerId, SlotIndex)) continue;

		if (IsItemAllowedInSlot(EquipmentContainerId, SlotIndex, ItemTag))
		{
			return SlotIndex;
		}
	}

	// If all matching slots are occupied, return the first matching occupied slot
	// so the normal move logic can swap.
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		if (!FindEntryAtSlot(EquipmentContainerId, SlotIndex)) continue;

		if (IsItemAllowedInSlot(EquipmentContainerId, SlotIndex, ItemTag))
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

bool USI_InventoryComponent::DoesItemMatchSlotProfile(const USI_ItemDefinition* ItemDefinition,
                                                      const USI_SlotProfile* SlotProfile) const
{
	if (!ItemDefinition || !SlotProfile) return false;

	// If the profile has allowed item types, the item's classification must match one of them.
	if (!SlotProfile->AllowedItemTypes.IsEmpty())
	{
		bool bMatchesAllowedType = false;

		for (const FGameplayTag& AllowedType : SlotProfile->AllowedItemTypes)
		{
			if (AllowedType.IsValid() && ItemDefinition->ItemType.MatchesTag(AllowedType))
			{
				bMatchesAllowedType = true;
				break;
			}
		}

		if (!bMatchesAllowedType) return false;
	}

	// If the profile has allowed equipment slots, the item's equip slot must match one of them.
	if (!SlotProfile->AllowedEquipmentSlots.IsEmpty())
	{
		bool bMatchesAllowedEquipmentSlot = false;

		for (const FGameplayTag& AllowedEquipmentSlot : SlotProfile->AllowedEquipmentSlots)
		{
			if (AllowedEquipmentSlot.IsValid() && ItemDefinition->EquipSlot.MatchesTag(AllowedEquipmentSlot))
			{
				bMatchesAllowedEquipmentSlot = true;
				break;
			}
		}

		if (!bMatchesAllowedEquipmentSlot) return false;
	}

	// If the profile has allowed item tags, the item must match at least one of them.
	if (!SlotProfile->AllowedItemTags.IsEmpty())
	{
		// Start as false until one allowed tag matches.
		bool bMatchesAllowedTag = false;

		// Check every allowed tag in the profile.
		for (const FGameplayTag& AllowedTag : SlotProfile->AllowedItemTags)
		{
			// MatchesTag allows parent tags to work.
			// Example: Item.Potion.Small matches Item.Potion.
			if (AllowedTag.IsValid() && ItemDefinition->ItemTag.MatchesTag(AllowedTag))
			{
				bMatchesAllowedTag = true;
				break;
			}
		}

		// If no allowed tag matched, reject the item.
		if (!bMatchesAllowedTag) return false;
	}

	// If all active rules passed, the item is allowed.
	return true;
}

void USI_InventoryComponent::ClearReferencesForItem(const FGameplayTag& ItemTag)
{
	if (!ItemTag.IsValid()) return;

	TMap<FGameplayTag, TArray<int32>> ChangedSlotsByContainer;

	for (int32 Index = Inventory.Entries.Num() - 1; Index >= 0; --Index)
	{
		const FSI_InventoryEntry& Entry = Inventory.Entries[Index];

		// Only remove shortcut entries for this item.
		if (!Entry.bIsReference) continue;
		if (Entry.ItemTag != ItemTag) continue;

		ChangedSlotsByContainer.FindOrAdd(Entry.ContainerId).AddUnique(Entry.SlotIndex);
		Inventory.Entries.RemoveAt(Index);
	}

	if (ChangedSlotsByContainer.IsEmpty()) return;

	Inventory.MarkArrayDirty();

	for (const TPair<FGameplayTag, TArray<int32>>& Pair : ChangedSlotsByContainer)
	{
		BroadcastSlotsChanged(Pair.Key, Pair.Value);
		ClientQueueSlotsChanged(Pair.Key, Pair.Value);
	}
}

void USI_InventoryComponent::RefreshReferencesForItem(const FGameplayTag& ItemTag)
{
	if (!ItemTag.IsValid()) return;

	const FGameplayTag MainContainerId = GetMainContainerId();
	if (!MainContainerId.IsValid()) return;

	// Find a real main inventory stack that references can point at.
	const FSI_InventoryEntry* MainInventoryEntry = nullptr;
	for (const FSI_InventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.bIsReference) continue;
		if (Entry.ContainerId != MainContainerId) continue;
		if (Entry.ItemTag != ItemTag) continue;
		if (Entry.Quantity <= 0) continue;

		MainInventoryEntry = &Entry;
		break;
	}

	// If the player no longer carries this item, remove every shortcut to it.
	if (!MainInventoryEntry)
	{
		ClearReferencesForItem(ItemTag);
		return;
	}

	TMap<FGameplayTag, TArray<int32>> ChangedSlotsByContainer;

	for (FSI_InventoryEntry& Entry : Inventory.Entries)
	{
		// References display main inventory quantity, so refresh every matching shortcut.
		if (!Entry.bIsReference) continue;
		if (Entry.ItemTag != ItemTag) continue;

		// Keep the reference pointed at a valid main inventory stack.
		Entry.SourceContainerId = MainContainerId;
		Entry.SourceSlotIndex = MainInventoryEntry->SlotIndex;
		Entry.ItemInstanceData = MainInventoryEntry->ItemInstanceData;
		Inventory.MarkItemDirty(Entry);

		ChangedSlotsByContainer.FindOrAdd(Entry.ContainerId).AddUnique(Entry.SlotIndex);
	}

	for (const TPair<FGameplayTag, TArray<int32>>& Pair : ChangedSlotsByContainer)
	{
		BroadcastSlotsChanged(Pair.Key, Pair.Value);
		ClientQueueSlotsChanged(Pair.Key, Pair.Value);
	}
}

void USI_InventoryComponent::UpdateItemEquippedStateForSlot(const FGameplayTag& ContainerId, int32 SlotIndex)
{
	if (!ContainerId.IsValid() || SlotIndex < 0) return;

	const FGameplayTag EquipmentContainerId = GetEquipmentContainerId();

	for (FSI_InventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.ContainerId != ContainerId) continue;
		if (Entry.SlotIndex != SlotIndex) continue;

		FSI_ItemInstanceData NewInstanceData = Entry.ItemInstanceData;
		if (!NewInstanceData.SourceItemTag.IsValid())
		{
			NewInstanceData.SourceItemTag = Entry.ItemTag;
		}

		if (!Entry.bIsReference && EquipmentContainerId.IsValid() && ContainerId == EquipmentContainerId)
		{
			NewInstanceData.bIsEquipped = true;
			NewInstanceData.EquippedContainerId = ContainerId;
			NewInstanceData.EquippedSlotIndex = SlotIndex;
		}
		else
		{
			NewInstanceData.ClearEquippedState();
		}

		const bool bChanged =
			Entry.ItemInstanceData.SourceItemTag != NewInstanceData.SourceItemTag ||
			Entry.ItemInstanceData.bIsEquipped != NewInstanceData.bIsEquipped ||
			Entry.ItemInstanceData.EquippedContainerId != NewInstanceData.EquippedContainerId ||
			Entry.ItemInstanceData.EquippedSlotIndex != NewInstanceData.EquippedSlotIndex;

		if (bChanged)
		{
			Entry.ItemInstanceData = NewInstanceData;
			Inventory.MarkItemDirty(Entry);

			UE_LOG(LogTemp, Warning,
				TEXT("SI ItemInstance equip state changed: Item=%s Container=%s Slot=%d bIsEquipped=%s EquippedContainer=%s EquippedSlot=%d"),
				*Entry.ItemTag.ToString(),
				*Entry.ContainerId.ToString(),
				Entry.SlotIndex,
				Entry.ItemInstanceData.bIsEquipped ? TEXT("true") : TEXT("false"),
				*Entry.ItemInstanceData.EquippedContainerId.ToString(),
				Entry.ItemInstanceData.EquippedSlotIndex);
		}

		return;
	}
}

int32 USI_InventoryComponent::GenerateRequestId()
{
	if (NextRequestId == INDEX_NONE)
	{
		NextRequestId = 1;
	}

	const int32 RequestId = NextRequestId++;

	if (NextRequestId <= 0)
	{
		NextRequestId = 1;
	}

	return RequestId;
}
