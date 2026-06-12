#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Interface/SI_InventoryActorInterface.h"
#include "SI_InventoryActor.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;

UCLASS()
class SYNCINVENTORY_API ASI_InventoryActor : public AActor, public ISI_InventoryActorInterface
{
	GENERATED_BODY()

public:
	ASI_InventoryActor();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void Highlight_Implementation() override;
	virtual void UnHighlight_Implementation() override;
	virtual FGameplayTag GetItemTag_Implementation() const override { return WorldItemTag; }
	virtual void SetItemTag_Implementation(const FGameplayTag NewTag) override;
	virtual int32 GetWorldQuantity_Implementation() const override { return WorldQuantity; }
	virtual void SetWorldQuantity_Implementation(int32 Quantity) override;
	virtual FSI_ItemInstanceData GetItemInstanceData_Implementation() const override { return WorldItemInstanceData; }
	virtual void SetItemInstanceData_Implementation(const FSI_ItemInstanceData& ItemInstanceData) override;

protected:
	UPROPERTY(EditInstanceOnly, Replicated, Category = "Inventory|Pickup", meta=(Categories="Item.Id"))
	FGameplayTag WorldItemTag;
	
	UPROPERTY(EditInstanceOnly, Replicated, Category = "Inventory|Pickup")
	int32 WorldQuantity = 1;

	UPROPERTY(EditInstanceOnly, Replicated, Category = "Inventory|Pickup")
	FSI_ItemInstanceData WorldItemInstanceData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UStaticMeshComponent> StaticMeshComp;

	UPROPERTY(EditAnywhere, Category="Inventory")
	TObjectPtr<UMaterialInterface> HighlightMaterial;
};
