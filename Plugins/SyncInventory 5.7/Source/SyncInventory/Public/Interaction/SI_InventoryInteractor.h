#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SI_InventoryInteractor.generated.h"

class APlayerController;
class APawn;
class UPrimitiveComponent;
class USphereComponent;
class USI_InventoryComponent;

UCLASS()
class SYNCINVENTORY_API USI_InventoryInteractor : public UObject
{
	GENERATED_BODY()
	
public:
	void Init(USI_InventoryComponent* InInventoryComponent, float InRadius);
	void PickupItem() const;
	
	UFUNCTION()
	void OnPossessedPawn(APawn* Old, APawn* NewPawn);
	
private:
	void CreateSphere(APawn* InPawn);
	void DestroySphere();
	
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
	
	UPROPERTY(Transient)
	TWeakObjectPtr<USI_InventoryComponent> InventoryComponent;
	
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> SphereComponent = nullptr;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> PlayerController;
	
	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> NearbyActors;
	
	float Radius;
};
