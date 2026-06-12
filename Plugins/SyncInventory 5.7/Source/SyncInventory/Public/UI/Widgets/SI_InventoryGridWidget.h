#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Templates/SubclassOf.h"
#include "SI_InventoryGridWidget.generated.h"


class UUniformGridPanel;
class USI_InventorySlotWidget;
class USI_InventoryUIManager;

UCLASS()
class SYNCINVENTORY_API USI_InventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitGridWidget(USI_InventoryUIManager* InInventoryUIManager, const FGameplayTag& InContainerId,
		const int32 InMaxSlots, int32 InColumns);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void BuildGrid();
	
	void RefreshSlots(const TArray<int32>& SlotIndices);

	void PressedFeedback(int32 SlotIndex);
	void ResetPressedFeedback(int32 SlotIndex);
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	TObjectPtr<USI_InventoryUIManager> InventoryUIManager = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|UI")
	TSubclassOf<USI_InventorySlotWidget> SlotWidgetClass = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|UI")
	TObjectPtr<UUniformGridPanel> SlotGridPanel = nullptr;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<USI_InventorySlotWidget>> SlotWidgets;
	
	UPROPERTY(Transient)
	FGameplayTag ContainerId;
	
	UPROPERTY(Transient)
	int32 MaxSlots;
	
	UPROPERTY(Transient)
	int32 Columns;
};
