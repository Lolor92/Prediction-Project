#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SI_ItemTypes.generated.h"

UENUM(BlueprintType)
enum class ESI_ItemRarity : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};

USTRUCT(BlueprintType)
struct FSI_ItemInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Instance", meta=(Categories="Item.Id"))
	FGameplayTag SourceItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Instance")
	bool bIsEquipped = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Instance", meta=(Categories="ContainerId"))
	FGameplayTag EquippedContainerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Instance")
	int32 EquippedSlotIndex = INDEX_NONE;

	bool IsEquipped() const
	{
		return bIsEquipped;
	}

	void ClearEquippedState()
	{
		bIsEquipped = false;
		EquippedContainerId = FGameplayTag();
		EquippedSlotIndex = INDEX_NONE;
	}
};
