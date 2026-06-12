#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "SI_ContainerConfig.generated.h"

class USI_SlotLayoutConfig;
class USI_WidgetBase;
class USI_ItemAddedNotificationWidget;

USTRUCT(BlueprintType)
struct FSI_ContainerConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(Categories="ContainerId"))
	FGameplayTag ContainerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bIsMainInventory = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bIsEquipmentInventory = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 MaxSlots = 100;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 Columns = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TSubclassOf<USI_WidgetBase> WidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Notifications")
	bool bShowItemAddedNotification = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Notifications",
		meta=(EditCondition="bShowItemAddedNotification", EditConditionHides))
	TSubclassOf<USI_ItemAddedNotificationWidget> ItemAddedNotificationWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	TObjectPtr<USI_SlotLayoutConfig> SlotLayoutConfig = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Rules")
	bool bAllowDeleteOnEmptyDrop = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Rules")
	bool bCreateReferenceOnCrossContainerDrop = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Rules")
	bool bStartVisibleOnSpawn = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Rules")
	bool bAffectsInputMode = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Rules")
	bool bCanReceiveInput = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Rules",
		meta=(EditCondition="bCanReceiveInput", EditConditionHides))
	bool bReceiveInputWhenWindowClosed = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Rules", meta=(Categories="Item.Type"))
	TArray<FGameplayTag> AllowedItemTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Rules", meta=(Categories="EquipmentSlot"))
	TArray<FGameplayTag> AllowedEquipmentSlots;
};
