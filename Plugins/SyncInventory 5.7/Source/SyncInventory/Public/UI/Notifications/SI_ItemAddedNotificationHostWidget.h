#pragma once

#include "CoreMinimal.h"
#include "SI_ItemAddedNotificationWidget.h"
#include "Blueprint/UserWidget.h"
#include "SI_ItemAddedNotificationHostWidget.generated.h"

class UCanvasPanel;
class USI_ItemAddedNotificationWidget;

USTRUCT()
struct FSI_ActiveItemAddedNotification
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USI_ItemAddedNotificationWidget> Widget = nullptr;

	FTimerHandle ExitTimerHandle;
	FTimerHandle RemoveTimerHandle;

	bool bIsExiting = false;
};

USTRUCT()
struct FSI_PendingItemAddedNotification
{
	GENERATED_BODY()

	UPROPERTY()
	FSI_ItemAddedNotificationData NotificationData;

	UPROPERTY()
	TSubclassOf<USI_ItemAddedNotificationWidget> WidgetClass;
};

UCLASS()
class SYNCINVENTORY_API USI_ItemAddedNotificationHostWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Notification")
	void QueueNotification(const FSI_ItemAddedNotificationData& NotificationData,
		TSubclassOf<USI_ItemAddedNotificationWidget> NotificationWidgetClass);
	
protected:
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|Notification")
	TObjectPtr<UCanvasPanel> NotificationCanvas = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Notification")
	FVector2D CanvasPosition = FVector2D(250.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Notification", meta=(ClampMin="1"))
	float StackSpacing = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Notification", meta=(ClampMin="0"))
	float IntroSeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Notification", meta=(ClampMin="0"))
	float HoldSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Notification", meta=(ClampMin="0"))
	float ExitSeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Notification", meta=(ClampMin="0"))
	float SpawnIntervalSeconds = 0.55f;

private:
	void ProcessNextQueuedNotification();
	void SpawnNotification(const FSI_PendingItemAddedNotification& PendingNotification);
	void RefreshStackPositions();
	void StartExitForWidget(USI_ItemAddedNotificationWidget* Widget);
	void RemoveNotificationWidget(USI_ItemAddedNotificationWidget* Widget);
	void ClearNotificationTimers();

	UPROPERTY()
	TArray<FSI_ActiveItemAddedNotification> ActiveNotifications;

	UPROPERTY()
	TArray<FSI_PendingItemAddedNotification> PendingNotifications;

	FTimerHandle SpawnQueueTimerHandle;
	bool bProcessingQueue = false;
};
