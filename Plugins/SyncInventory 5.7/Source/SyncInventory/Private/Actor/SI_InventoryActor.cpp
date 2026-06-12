#include "Actor/SI_InventoryActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"


ASI_InventoryActor::ASI_InventoryActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComp");
	RootComponent = StaticMeshComp.Get();
	
	WorldQuantity = 1;
}

void ASI_InventoryActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, WorldItemTag);
	DOREPLIFETIME(ThisClass, WorldQuantity);
	DOREPLIFETIME(ThisClass, WorldItemInstanceData);
}

void ASI_InventoryActor::Highlight_Implementation()
{
	if (!StaticMeshComp || !HighlightMaterial) return;
	
	StaticMeshComp->SetOverlayMaterial(HighlightMaterial);
}

void ASI_InventoryActor::UnHighlight_Implementation()
{
	if (!StaticMeshComp) return;
	
	StaticMeshComp->SetOverlayMaterial(nullptr);
}

void ASI_InventoryActor::SetItemTag_Implementation(const FGameplayTag NewTag)
{
	if (!HasAuthority()) return;
	
	WorldItemTag = NewTag;
	ForceNetUpdate();
}

void ASI_InventoryActor::SetWorldQuantity_Implementation(int32 Quantity)
{
	if (!HasAuthority()) return;
	
	if (Quantity <= 0)
	{
		Destroy();
		return;
	}

	WorldQuantity = Quantity;
	ForceNetUpdate();
}

void ASI_InventoryActor::SetItemInstanceData_Implementation(const FSI_ItemInstanceData& ItemInstanceData)
{
	if (!HasAuthority()) return;

	WorldItemInstanceData = ItemInstanceData;
	if (!WorldItemInstanceData.SourceItemTag.IsValid())
	{
		WorldItemInstanceData.SourceItemTag = WorldItemTag;
	}

	ForceNetUpdate();
}
