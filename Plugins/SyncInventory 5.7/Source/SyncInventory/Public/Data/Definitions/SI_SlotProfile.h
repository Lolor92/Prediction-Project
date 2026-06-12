#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SI_SlotProfile.generated.h"

/*
 * A reusable rule set for one kind of inventory slot.
 *
 * Examples:
 * - AnyItem
 * - PotionOnly
 * - Helmet
 * - MainHand
 */

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class SYNCINVENTORY_API USI_SlotProfile : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// Stable id used by layout configs to reference this profile.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Profile")
	FName ProfileId;
	
	// Allowed item type tags, such as Item.Type.Consumable.
	// Empty means this profile does not restrict by item type.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Rules", meta = (Categories = "Item.Type"))
	TArray<FGameplayTag> AllowedItemTypes;

	// Allowed equipment slot tags, such as EquipmentSlot.MainHand.
	// Empty means this profile does not restrict by equipment slot.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Rules", meta = (Categories = "EquipmentSlot"))
	TArray<FGameplayTag> AllowedEquipmentSlots;
	
	// Allowed exact item tags, such as Item.Id.Consumable.SmallPotion.
	// Empty means this profile does not restrict by exact item tag.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Rules", meta = (Categories = "Item.Id"))
	TArray<FGameplayTag> AllowedItemTags;
};
