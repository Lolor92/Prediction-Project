#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/Databases/SI_SlotProfileRegistry.h"
#include "SI_SlotLayoutConfig.generated.h"

/*
 *Short version: this class is the asset that says slot 0 uses this profile,
 *slot 1 uses that profile, and it also helps the editor show valid profile IDs in a dropdown.
 */

/*
 * One row of layout data for one inventory slot.
 *
 * Think of this as:
 * "Slot index 0 uses the MainHand profile."
 * "Slot index 1 uses the Helmet profile."
 */
USTRUCT(BlueprintType)
struct FSI_SlotLayoutEntry
{
	GENERATED_BODY()

	// The actual inventory slot number this entry controls.
	// Example: 0 means the first slot.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Slot", meta = (ClampMin = "0"))
	int32 SlotIndex = INDEX_NONE;

	// The profile id this slot should use.
	// This should match a ProfileId inside the SlotProfileRegistry.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Slot", meta = (GetOptions = "GetAvailableProfileIds"))
	FName SlotProfileId;

	// Optional gameplay input tag for action bar / hotbar slots.
	// Normal inventory slots can leave this empty.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	// If false, the UI can place the slot using normal grid order.
	// If true, this slot uses GridRow and GridColumn below.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Grid")
	bool bOverrideGridPosition = false;

	// Custom row position for this slot.
	// Only used when bOverrideGridPosition is true.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Grid", meta = (ClampMin = "0", EditCondition = "bOverrideGridPosition", EditConditionHides))
	int32 GridRow = 0;

	// Custom column position for this slot.
	// Only used when bOverrideGridPosition is true.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Grid", meta = (ClampMin = "0", EditCondition = "bOverrideGridPosition", EditConditionHides))
	int32 GridColumn = 0;
};

/*
 * A data asset that describes how slots are configured for a container.
 *
 * Example:
 * Player equipment container uses Helmet, Chest, Boots profiles.
 * Backpack container may use AnyItem for every slot.
 */
UCLASS(BlueprintType)
class SYNCINVENTORY_API USI_SlotLayoutConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// This registry contains the reusable slot profiles this layout can choose from.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Profiles")
	TObjectPtr<USI_SlotProfileRegistry> SlotProfileRegistry = nullptr;

	// Each entry describes one slot in this layout.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Layout", meta = (TitleProperty = "SlotIndex"))
	TArray<FSI_SlotLayoutEntry> SlotEntries;

	// Gives the editor a dropdown list of available ProfileId values.
	UFUNCTION()
	TArray<FString> GetAvailableProfileIds() const;

	// Finds the layout entry for a specific slot index.
	const FSI_SlotLayoutEntry* FindSlotEntryByIndex(int32 SlotIndex) const;

#if WITH_EDITOR
	// Runs Unreal editor validation for this data asset.
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	// Runs after a property changes in the editor.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
