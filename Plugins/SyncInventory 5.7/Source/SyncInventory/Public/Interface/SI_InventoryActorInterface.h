#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/Item/SI_ItemTypes.h"
#include "UObject/Interface.h"
#include "SI_InventoryActorInterface.generated.h"


UINTERFACE()
class USI_InventoryActorInterface : public UInterface
{
	GENERATED_BODY()
};


class SYNCINVENTORY_API ISI_InventoryActorInterface
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Interaction")
	void Highlight();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Interaction")
	void UnHighlight();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Interaction")
	FGameplayTag GetItemTag()const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Interaction")
	void SetItemTag(const FGameplayTag NewTag);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Interaction")
	int32 GetWorldQuantity() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Interaction")
	void SetWorldQuantity(int32 Quantity);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Interaction")
	FSI_ItemInstanceData GetItemInstanceData() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Interaction")
	void SetItemInstanceData(const FSI_ItemInstanceData& ItemInstanceData);
};
