#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/Item/SI_ItemTypes.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Templates/SubclassOf.h"
#include "SI_ItemDefinition.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UStaticMesh;
class UTexture2D;

USTRUCT(BlueprintType)
struct FSI_ItemStatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Stats", meta = (Categories = "Stat"))
	FGameplayTag StatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Stats")
	float Magnitude = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Stats")
	TEnumAsByte<EGameplayModOp::Type> Operation = EGameplayModOp::Additive;
};

UCLASS(BlueprintType)
class SYNCINVENTORY_API USI_ItemDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	USI_ItemDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Identity")
	FText ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Identity", meta=(Categories="Item.Id"))
	FGameplayTag ItemTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Presentation")
	UTexture2D* ItemIcon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Presentation", meta=(Categories="Item.Rarity", DisplayName="Item Rarity"))
	FGameplayTag ItemRarityTag;

	UPROPERTY()
	ESI_ItemRarity ItemRarity = ESI_ItemRarity::Common;

	UFUNCTION(BlueprintPure, Category="Inventory|Presentation")
	FGameplayTag GetItemRarityTag() const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Classification",
	meta=(Categories="Item.Type"))
	FGameplayTag ItemType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Progression", meta=(ClampMin="1"))
	int32 ItemLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Progression", meta=(ClampMin="0"))
	int32 RequiredLevel = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Binding")
	bool bBoundToCharacter = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Tooltip", meta=(MultiLine="true"))
	FText Description;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Stacking")
	bool bStackable = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Stacking",
		meta=(ClampMin="1", EditCondition="bStackable", EditConditionHides))
	int32 MaxStackSize = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Usage")
	bool bCanUse = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Usage",
		meta=(EditCondition="bCanUse", EditConditionHides))
	bool bConsumeOnUse = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Usage",
		meta=(EditCondition="bConsumeOnUse", EditConditionHides))
	int32 AmountToConsume = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equip")
	bool bEquippable = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equip",
		meta=(Categories="EquipmentSlot", EditCondition="bEquippable", EditConditionHides))
	FGameplayTag EquipSlot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equip|Stats",
		meta = (EditCondition = "bEquippable", EditConditionHides, TitleProperty = "StatTag"))
	TArray<FSI_ItemStatModifier> EquipStats;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equip",
	meta=(EditCondition="bEquippable", EditConditionHides))
	TObjectPtr<UStaticMesh> MeshToSpawn = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equip",
		meta=(EditCondition="bEquippable", EditConditionHides))
	FName SocketName = NAME_None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equip",
	meta=(EditCondition="bEquippable", EditConditionHides))
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equip",
		meta=(EditCondition="bEquippable", EditConditionHides))
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equip",
		meta=(EditCondition="bEquippable", EditConditionHides))
	FVector RelativeScale = FVector::OneVector;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Usage|Effects",
		meta=(EditCondition="bCanUse", EditConditionHides))
	TArray<TSubclassOf<UGameplayEffect>> UseGameplayEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Usage|Cooldown",
		meta=(EditCondition="bCanUse", EditConditionHides))
	TSubclassOf<UGameplayEffect> CooldownGameplayEffect;

	// When true, using this item applies CooldownGameplayEffect from the inventory system.
	// Turn this off for abilities that apply their own cooldown but still need the inventory UI to track it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Usage|Cooldown",
		meta=(EditCondition="bCanUse && CooldownGameplayEffect", EditConditionHides))
	bool bApplyCooldownOnUse = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Ability")
	TArray<TSubclassOf<UGameplayAbility>> GameplayAbilities;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
