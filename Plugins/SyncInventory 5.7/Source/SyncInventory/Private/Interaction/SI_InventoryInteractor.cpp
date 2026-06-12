#include "Interaction/SI_InventoryInteractor.h"
#include "Components/SphereComponent.h"
#include "Component/SI_InventoryComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interface/SI_InventoryActorInterface.h"

void USI_InventoryInteractor::Init(USI_InventoryComponent* InInventoryComponent, float InRadius)
{
	InventoryComponent = InInventoryComponent;
	Radius = InRadius;
	
	if (!InventoryComponent.IsValid()) return;
	
	PlayerController = Cast<APlayerController>(InventoryComponent->GetOwner());
	if (!PlayerController.IsValid()) return;
	
	PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::OnPossessedPawn);
	PlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawn);
	
	OnPossessedPawn(nullptr, PlayerController->GetPawn<APawn>());
}

void USI_InventoryInteractor::PickupItem() const
{
	if (!InventoryComponent.IsValid()) return;
	if (!PlayerController.IsValid()) return;
	if (!PlayerController->HasAuthority()) return;
	
	UWorld* World = PlayerController->GetWorld();
	if (!World) return;
	
	APawn* Pawn = PlayerController->GetPawn<APawn>();
	if (!Pawn) return;
	
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);
	
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(Pawn);
	
	TArray<FOverlapResult> Overlaps;
	const bool bHit = World->OverlapMultiByObjectType(Overlaps, Pawn->GetActorLocation(), FQuat::Identity,
		ObjParams, FCollisionShape::MakeSphere(Radius), QueryParams);
	
	if (!bHit) return;
	
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlappedActor = Overlap.GetActor();
		if (!IsValid(OverlappedActor)) continue;
		if (!OverlappedActor->GetClass()->ImplementsInterface(USI_InventoryActorInterface::StaticClass())) continue;
		
		const FGameplayTag ItemTag = ISI_InventoryActorInterface::Execute_GetItemTag(OverlappedActor);
		const int32 Quantity = ISI_InventoryActorInterface::Execute_GetWorldQuantity(OverlappedActor);
		
		if (!ItemTag.IsValid() || Quantity <= 0) continue;
		
		const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
		const USI_ItemDefinition* ItemDefinition = ItemDatabase ? ItemDatabase->GetItemDefinitionByTag(ItemTag) : nullptr;
		if (!ItemDefinition) continue;
		
		FGameplayTag ContainerId = InventoryComponent->GetMainContainerId();
		if (!ContainerId.IsValid()) continue;
		
		FSI_AddItemRequest Request;
		Request.ContainerId = ContainerId;
		Request.ItemTag = ItemTag;
		Request.Quantity = Quantity;
		Request.RequestId = InventoryComponent->GenerateRequestId();
		Request.ItemInstanceData = ISI_InventoryActorInterface::Execute_GetItemInstanceData(OverlappedActor);
		if (!Request.ItemInstanceData.SourceItemTag.IsValid())
		{
			Request.ItemInstanceData.SourceItemTag = ItemTag;
		}
		
		FSI_AddItemResponse Response;
		const bool bAddedAny = InventoryComponent->TryAddItemAuthority(Request, Response);
		if (PlayerController->IsLocalController())
		{
			InventoryComponent->OnAddItemRequestCompleted.Broadcast(Response);
		}
		else
		{
			InventoryComponent->ClientReceiveAddItemResponse(Response);
		}
		
		if (!bAddedAny || Response.AddedQuantity <= 0) continue;
		
		ISI_InventoryActorInterface::Execute_SetWorldQuantity(OverlappedActor, Response.RemainingQuantity);
	}
}

void USI_InventoryInteractor::OnPossessedPawn(APawn* Old, APawn* NewPawn)
{
	if (NewPawn == nullptr) return;
	
	CreateSphere(NewPawn);
}

void USI_InventoryInteractor::CreateSphere(APawn* InPawn)
{
	if (InPawn == nullptr) return;
	if (!PlayerController.IsValid() || !PlayerController->IsLocalController()) return;
	
	DestroySphere();
	
	SphereComponent = NewObject<USphereComponent>(InPawn, TEXT("InteractionSphere"));
	if (!SphereComponent) return;
	
	InPawn->AddInstanceComponent(SphereComponent);
	
	SphereComponent->SetupAttachment(InPawn->GetRootComponent());
	SphereComponent->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	SphereComponent->InitSphereRadius(Radius);
	SphereComponent->SetGenerateOverlapEvents(true);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SphereComponent->RegisterComponent();
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereBeginOverlap);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	SphereComponent->UpdateOverlaps();
}

void USI_InventoryInteractor::DestroySphere()
{
	if (!SphereComponent) return;

	SphereComponent->OnComponentBeginOverlap.RemoveAll(this);
	SphereComponent->OnComponentEndOverlap.RemoveAll(this);
	SphereComponent->DestroyComponent();
	SphereComponent = nullptr;
}

void USI_InventoryInteractor::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || NearbyActors.Contains(OtherActor)) return;
	if (!OtherActor->GetClass()->ImplementsInterface(USI_InventoryActorInterface::StaticClass())) return;

	ISI_InventoryActorInterface::Execute_Highlight(OtherActor);
	NearbyActors.Add(OtherActor);
}

void USI_InventoryInteractor::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsValid(OtherActor) || !NearbyActors.Contains(OtherActor)) return;

	if (OtherActor->GetClass()->ImplementsInterface(USI_InventoryActorInterface::StaticClass()))
	{
		ISI_InventoryActorInterface::Execute_UnHighlight(OtherActor);
	}

	NearbyActors.Remove(OtherActor);
}
