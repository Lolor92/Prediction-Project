#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Types/Structs/SI_CooldownTypes.h"
#include "SI_InventorySlotDragVisual.generated.h"

class USI_InventoryComponent;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class SYNCINVENTORY_API USI_InventorySlotDragVisual : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitDragVisual(USI_InventoryComponent* InInventoryComponent, const FGameplayTag& InContainerId, const int32 InSlotIndex);
	
protected:
	void SetToDefault();
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> SlotBackgroundImage = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> CooldownOverlayImage = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> DeactivatedImageOverlay = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush DefaultSlotBackgroundBrush;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity", meta=(Categories="Item.Rarity"))
	TMap<FGameplayTag, FSlateBrush> RaritySlotBackgroundBrushes;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush CommonSlotBackgroundBrush;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush RareSlotBackgroundBrush;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush EpicSlotBackgroundBrush;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Rarity")
	FSlateBrush LegendarySlotBackgroundBrush;
	
	UPROPERTY(EditDefaultsOnly, Category="Inventory|UI|Cooldown")
	TObjectPtr<UMaterialInterface> CooldownMaterial = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|UI|Cooldown")
	FName CooldownProgressParameterName = TEXT("Progress");
	
private:
	const FSlateBrush& GetBackgroundRarityBrush(const FGameplayTag& RarityTag) const;
	
	void ApplyCooldownVisual(const FSI_SlotCooldownState& CooldownState);
	
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownMaterialInstance = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_InventoryComponent> InventoryComponent = nullptr;
	
	UPROPERTY(Transient)
	FGameplayTag ContainerId;
	
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;
};
