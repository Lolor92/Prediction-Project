#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/Item/SI_ItemTypes.h"
#include "SI_RequestTypes.generated.h"

USTRUCT(BlueprintType)
struct FSI_AddItemRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request")
	int32 RequestId = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request", meta=(Categories="ContainerId"))
	FGameplayTag ContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request", meta=(Categories="Item.Id"))
	FGameplayTag ItemTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request")
	FSI_ItemInstanceData ItemInstanceData;
};

UENUM(BlueprintType)
enum class ESI_AddItemResult : uint8
{
	FullSuccess,
	PartialSuccess,
	Failed
};

USTRUCT(BlueprintType)
struct FSI_AddItemResponse
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RequestId = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="ContainerId"))
	FGameplayTag ContainerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="Item.Id"))
	FGameplayTag ItemTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	ESI_AddItemResult Result = ESI_AddItemResult::Failed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RequestedQuantity = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 AddedQuantity = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RemainingQuantity = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TArray<int32> SlotsToUpdate;
};

USTRUCT(BlueprintType)
struct FSI_RemoveItemRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request")
	int32 RequestId = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="ContainerId"))
	FGameplayTag ContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 SlotIndex = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FSI_RemoveItemFromContainerRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request")
	int32 RequestId = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request", meta=(Categories="ContainerId"))
	FGameplayTag ContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request", meta=(Categories="Item.Id"))
	FGameplayTag ItemTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request")
	bool bRemoveAll = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request", meta=(ClampMin="1", EditCondition="!bRemoveAll", EditConditionHides))
	int32 Quantity = 1;
};

UENUM(BlueprintType)
enum class ESI_RemoveItemResult : uint8
{
	FullSuccess,
	PartialSuccess,
	Failed
};

USTRUCT(BlueprintType)
struct FSI_RemoveItemResponse
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RequestId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="ContainerId"))
	FGameplayTag ContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="Item.Id"))
	FGameplayTag ItemTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	ESI_RemoveItemResult Result = ESI_RemoveItemResult::Failed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RequestedQuantity = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RemovedQuantity = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RemainingQuantity = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RemainingInSlot = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TArray<int32> SlotsToUpdate;
};

USTRUCT(BlueprintType)
struct FSI_MoveItemRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request")
	int32 RequestId = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="ContainerId"))
	FGameplayTag FromContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 FromSlotIndex = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="ContainerId"))
	FGameplayTag ToContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ToSlotIndex = INDEX_NONE;
};

UENUM(BlueprintType)
enum class ESI_MoveItemResult : uint8
{
	Moved,
	Merged,
	Swapped,
	Failed
};

UENUM(BlueprintType)
enum class ESI_InventoryFailureReason : uint8
{
	None,
	Unknown,
	LevelRequirementNotMet
};

USTRUCT(BlueprintType)
struct FSI_MoveItemResponse
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RequestId = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="Item.Id"))
	FGameplayTag ItemTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	ESI_MoveItemResult Result = ESI_MoveItemResult::Failed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	ESI_InventoryFailureReason FailureReason = ESI_InventoryFailureReason::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="ContainerId"))
	FGameplayTag FromContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 FromSlotIndex = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="ContainerId"))
	FGameplayTag ToContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ToSlotIndex = INDEX_NONE;
	
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TArray<int32> SlotsToUpdate;
};

USTRUCT(BlueprintType)
struct FSI_SplitStackRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Request")
	int32 RequestId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta=(Categories="ContainerId"))
	FGameplayTag ContainerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 SplitQuantity = 1;
};

UENUM(BlueprintType)
enum class ESI_SplitStackResult : uint8
{
	Success,
	Failed
};

USTRUCT(BlueprintType)
struct FSI_SplitStackResponse
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RequestId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="Item.Id"))
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	ESI_SplitStackResult Result = ESI_SplitStackResult::Failed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta=(Categories="ContainerId"))
	FGameplayTag ContainerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 NewSlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 SplitQuantity = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TArray<int32> SlotsToUpdate;
};
