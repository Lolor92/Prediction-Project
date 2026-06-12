#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"
#include "Types/Requests/SI_RequestTypes.h"
#include "SI_InventoryUIManager.generated.h"


class UAbilitySystemComponent;
class USI_SplitStackWidget;
class USI_DeleteItemWidget;
class USI_WidgetBase;
class USI_PlayerInventoryWidget;
class USI_InventoryComponent;
class USI_ItemAddedNotificationHostWidget;
class USI_ItemAddedNotificationWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SYNCINVENTORY_API USI_InventoryUIManager : public UActorComponent
{
	GENERATED_BODY()

public:
	USI_InventoryUIManager();
	
	UAbilitySystemComponent* GetAbilitySystemComponent();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	USI_InventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void ToggleWidget(const FGameplayTag& ContainerId);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void OpenWidget(const FGameplayTag& ContainerId);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void CloseWidget(const FGameplayTag& ContainerId);
	
	void OpenDeleteItemWidget(const FGameplayTag& ContainerId, const int32 SlotIndex, const FVector2D& ScreenPosition);
	void CloseDeleteItemWidget();
	void OpenSplitStackWidget(const FGameplayTag& ContainerId, const int32 SlotIndex, const FVector2D& ScreenPosition);
	void CloseSplitStackWidget();
	
	UFUNCTION(BlueprintCallable, Category="Inventory|Input")
	void HandleInputPressed(FGameplayTag InputTag);
	
	UFUNCTION(BlueprintCallable, Category="Inventory|Input")
	void HandleInputReleased(FGameplayTag InputTag);

protected:
	UFUNCTION()
	void InitializeUIManager();
	
	UFUNCTION()
	void HandleSlotsChanged(const FGameplayTag ContainerTag, const TArray<int32>& SlotIndices);
	
	UFUNCTION()
	void HandleAddItemRequestCompleted(const FSI_AddItemResponse& Response);

	UFUNCTION()
	void HandleMoveItemRequestCompleted(const FSI_MoveItemResponse& Response);

private:
	void HandleGameplayTagChanged(FGameplayTag ChangedTag, int32 NewCount);
	
	void UpdateInputMode(bool bInventoryOpen);
	void RefreshInputMode();
	
	void EnsureItemAddedNotificationHost();
	void QueueInventoryMessageNotification(const FText& MessageText);

	UPROPERTY(EditAnywhere, Category = "Inventory|Notifications")
	TSubclassOf<USI_ItemAddedNotificationHostWidget> ItemAddedNotificationHostWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Inventory|Notifications")
	TSubclassOf<USI_ItemAddedNotificationWidget> InventoryMessageNotificationWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Inventory|Notifications")
	int32 ItemAddedNotificationZOrder = 90;

	UPROPERTY(Transient)
	TObjectPtr<USI_ItemAddedNotificationHostWidget> ActiveItemAddedNotificationHostWidget = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_InventoryComponent> InventoryComponent = nullptr;

	UPROPERTY(Transient)
	TSubclassOf<USI_DeleteItemWidget> DeleteItemWidgetClass;

	UPROPERTY(Transient)
	TSubclassOf<USI_SplitStackWidget> SplitStackWidgetClass;
	
	UPROPERTY(Transient)
	TMap <FGameplayTag, TObjectPtr<USI_WidgetBase>> ActiveWidget;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_DeleteItemWidget> ActiveDeleteItemWidget = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<USI_SplitStackWidget> ActiveSplitStackWidget = nullptr;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
