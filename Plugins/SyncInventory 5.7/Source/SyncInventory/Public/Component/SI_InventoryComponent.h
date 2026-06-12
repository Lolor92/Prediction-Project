#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Databases/SI_ItemDatabase.h"
#include "Data/Databases/SI_ItemStatRegistry.h"
#include "Data/Databases/SI_SlotLayoutConfig.h"
#include "Data/Definitions/SI_SlotProfile.h"
#include "Mutator/SI_InventoryMutator.h"
#include "Types/Requests/SI_RequestTypes.h"
#include "Replication/SI_InventoryFastArray.h"
#include "Types/Config/SI_ContainerConfig.h"
#include "GameplayEffectTypes.h"
#include "SI_InventoryComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class USI_InventoryInteractor;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSlotChanged, FGameplayTag /*ContainerTag*/, const TArray<int32>& /*SlotIndices*/)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSI_OnAddItemRequestCompleted, const FSI_AddItemResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSI_OnRemoveItemRequestCompleted, const FSI_RemoveItemResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSI_OnMoveItemRequestCompleted, const FSI_MoveItemResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSI_OnSplitItemRequestCompleted, const FSI_SplitStackResponse&, Response);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="SI Inventory Component"))
class SYNCINVENTORY_API USI_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USI_InventoryComponent();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	int32 GetContainerMaxSlots(const FGameplayTag& ContainerId) const;
	int32 GetItemMaxStackSize(const FGameplayTag& ItemTag) const;
	const FSI_ContainerConfig* GetContainerConfig(const FGameplayTag& ContainerId) const;
	const TArray<FSI_ContainerConfig>& GetContainerConfigs() const { return ContainerConfigs; }
	const FSI_InventoryEntry* FindEntryAtSlot(const FGameplayTag& ContainerId, int32 SlotIndex) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Inventory|Items")
	bool GetItemInstanceDataAtSlot(const FGameplayTag& ContainerId, int32 SlotIndex, FSI_ItemInstanceData& OutItemInstanceData) const;
	int32 GetMainInventoryQuantityForItem(const FGameplayTag& ItemTag) const;
	const USI_ItemDatabase* GetItemDatabase() { return ItemDatabase; }
	FGameplayTag GetMainContainerId() const;
	FGameplayTag GetEquipmentContainerId() const;
	const USI_SlotLayoutConfig* GetSlotLayoutConfig(const FGameplayTag& ContainerId) const;
	const USI_SlotProfile* GetSlotProfileForSlot(const FGameplayTag& ContainerId, int32 SlotIndex) const;
	const FSI_SlotLayoutEntry* GetSlotLayoutEntry(const FGameplayTag& ContainerId, int32 SlotIndex) const;
	UAbilitySystemComponent* GetAbilitySystemComponent();
	UFUNCTION(BlueprintCallable, Category="Inventory|Character")
	void SetCharacterLevel(int32 NewCharacterLevel);
	UFUNCTION(Server, Reliable)
	void ServerSetCharacterLevel(int32 NewCharacterLevel);
	UFUNCTION(BlueprintPure, Category="Inventory|Character")
	int32 GetCharacterLevel() const { return CharacterLevel; }
	UFUNCTION(BlueprintPure, Category="Inventory|Character")
	bool DoesCharacterMeetItemLevelRequirement(const FGameplayTag& ItemTag) const;
	bool IsItemAllowedInContainer(const FGameplayTag& ContainerId, const FGameplayTag& ItemTag) const;
	bool IsItemAllowedInSlot(const FGameplayTag& ContainerId, int32 SlotIndex, const FGameplayTag& ItemTag) const;
	int32 FindFirstFreeAllowedSlot(const FGameplayTag& ContainerId, const FGameplayTag& ItemTag) const;
	int32 FindBestEquipmentSlotForItem(const FGameplayTag& ItemTag) const;
	
	int32 GenerateRequestId();
	
	// Add Item
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	int32 RequestAddItem(FSI_AddItemRequest Request);
	UFUNCTION(Server, Reliable)
	void ServerRequestAddItem(const FSI_AddItemRequest& Request);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Authority")
	bool TryAddItemAuthority(const FSI_AddItemRequest& Request, FSI_AddItemResponse& OutResponse);
	
	// Remove Item
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	int32 RequestRemoveItem(FSI_RemoveItemRequest Request);
	UFUNCTION(Server, Reliable)
	void ServerRequestRemoveItem(const FSI_RemoveItemRequest& Request);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Authority")
	bool TryRemoveItemAuthority(const FSI_RemoveItemRequest& Request, FSI_RemoveItemResponse& OutResponse);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request", meta=(DisplayName="Request Remove Item From Container"))
	int32 RequestRemoveItemFromContainer(FSI_RemoveItemFromContainerRequest Request);
	UFUNCTION(Server, Reliable)
	void ServerRequestRemoveItemFromContainer(const FSI_RemoveItemFromContainerRequest& Request);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Authority", meta=(DisplayName="Try Remove Item From Container Authority"))
	bool TryRemoveItemFromContainerAuthority(const FSI_RemoveItemFromContainerRequest& Request, FSI_RemoveItemResponse& OutResponse);
	
	// Move Item
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	int32 RequestMoveItem(FSI_MoveItemRequest Request);
	UFUNCTION(Server, Reliable)
	void ServerRequestMoveItem(const FSI_MoveItemRequest& Request);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	bool TryMoveItemAuthority(const FSI_MoveItemRequest& Request, FSI_MoveItemResponse& OutResponse);
	
	// SplitItem
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	int32 RequestSplitItem(FSI_SplitStackRequest Request);
	UFUNCTION(Server, Reliable)
	void ServerRequestSplitItem(const FSI_SplitStackRequest& Request);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	bool TrySplitItemAuthority(const FSI_SplitStackRequest& Request, FSI_SplitStackResponse& OutResponse);
	
	// Pickup Item
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	void RequestPickupItem();
	UFUNCTION(Server, Reliable)
	void ServerRequestPickupItem();
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	void TryPickupItemAuthority();
	
	// UseItem
	UFUNCTION(BlueprintCallable, Category = "Inventory|Request")
	void RequestUseItem(const FGameplayTag& ContainerId, int32 SlotIndex, bool bPressed);
	UFUNCTION(Server, Reliable)
	void ServerRequestUseItem(const FGameplayTag& ContainerId, int32 SlotIndex, bool bPressed);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Authority")
	bool TryUseItemAuthority(const FGameplayTag& ContainerId, int32 SlotIndex, bool bPressed);
	
	// Replication callbacks from FastArray
	void HandleReplicationAdd(const TArrayView<int32>& Indices);
	void HandleReplicationRemove(const TArray<FSI_InventoryEntry>& RemovedEntries);
	void HandleReplicationChange(const TArrayView<int32>& Indices);
	
	void BroadcastSlotsChanged(const FGameplayTag& ContainerTag, const TArray<int32>& SlotIndices);
	
	UFUNCTION(Client, Reliable)
	void ClientQueueSlotsChanged(const FGameplayTag& ContainerId, const TArray<int32>& SlotIndices);
	void FlushPendingClientSlotRefreshes();
	
	UFUNCTION(Client, Reliable)
	void ClientReceiveAddItemResponse(const FSI_AddItemResponse& Response);

	UFUNCTION(Client, Reliable)
	void ClientReceiveRemoveItemResponse(const FSI_RemoveItemResponse& Response);

	UFUNCTION(Client, Reliable)
	void ClientReceiveMoveItemResponse(const FSI_MoveItemResponse& Response);

	UFUNCTION(Client, Reliable)
	void ClientReceiveSplitItemResponse(const FSI_SplitStackResponse& Response);
	
	FOnSlotChanged OnSlotChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FSI_OnAddItemRequestCompleted OnAddItemRequestCompleted;

	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FSI_OnRemoveItemRequestCompleted OnRemoveItemRequestCompleted;

	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FSI_OnMoveItemRequestCompleted OnMoveItemRequestCompleted;

	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FSI_OnSplitItemRequestCompleted OnSplitItemRequestCompleted;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(TitleProperty="ContainerId"))
	TArray<FSI_ContainerConfig> ContainerConfigs;
	
	UPROPERTY(Replicated)
	FSI_InventoryEntryArray Inventory;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<USI_ItemDatabase> ItemDatabase = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Equip|Stats")
	TObjectPtr<USI_ItemStatRegistry> ItemStatRegistry = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Inventory|Character", meta=(ClampMin="1"))
	int32 CharacterLevel = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory|Interaction", meta=(ClampMin="0"))
	float InteractionRadius = 200.f;
	
private:
	bool DoesItemMatchSlotProfile(const USI_ItemDefinition* ItemDefinition, const USI_SlotProfile* SlotProfile) const;
	void ClearReferencesForItem(const FGameplayTag& ItemTag);
	void RefreshReferencesForItem(const FGameplayTag& ItemTag);
	void UpdateItemEquippedStateForSlot(const FGameplayTag& ContainerId, int32 SlotIndex);
	void RebuildEquipmentStatsForSlots(const TArray<int32>& SlotIndices);
	void RemoveEquipmentStatsFromSlot(int32 SlotIndex);
	void ApplyEquipmentStatsFromSlot(int32 SlotIndex);
	void RebuildEquipmentVisualsForSlots(const TArray<int32>& SlotIndices);
	void RebuildAllEquipmentVisuals();
	void RemoveEquipmentVisualFromSlot(int32 SlotIndex);
	void ApplyEquipmentVisualFromSlot(int32 SlotIndex);
	
	UPROPERTY(Transient)
	int32 NextRequestId = 1;
	
	FSI_InventoryMutator InventoryMutator;
	
	UPROPERTY()
	TObjectPtr<USI_InventoryInteractor> InventoryInteractor;
	
	TMap<FGameplayTag, TArray<int32>> PendingClientSlotRefreshes;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TMap<int32, FActiveGameplayEffectHandle> ActiveEquipmentStatEffects;
	
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UStaticMeshComponent>> ActiveEquipmentMeshes;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UGameplayEffect>> ActiveEquipmentStatEffectDefinitions;
};
