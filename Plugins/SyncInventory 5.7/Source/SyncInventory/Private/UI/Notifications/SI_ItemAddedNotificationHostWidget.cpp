#include "UI/Notifications/SI_ItemAddedNotificationHostWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"

void USI_ItemAddedNotificationHostWidget::QueueNotification(const FSI_ItemAddedNotificationData& NotificationData,
	TSubclassOf<USI_ItemAddedNotificationWidget> NotificationWidgetClass)
{
	if (!NotificationWidgetClass) return;

	FSI_PendingItemAddedNotification PendingNotification;
	PendingNotification.NotificationData = NotificationData;
	PendingNotification.WidgetClass = NotificationWidgetClass;

	PendingNotifications.Add(PendingNotification);

	if (!bProcessingQueue)
	{
		ProcessNextQueuedNotification();
	}
}

void USI_ItemAddedNotificationHostWidget::NativeDestruct()
{
	ClearNotificationTimers();

	Super::NativeDestruct();
}

void USI_ItemAddedNotificationHostWidget::ProcessNextQueuedNotification()
{
	if (PendingNotifications.IsEmpty())
	{
		bProcessingQueue = false;
		return;
	}

	bProcessingQueue = true;

	const FSI_PendingItemAddedNotification PendingNotification = PendingNotifications[0];
	PendingNotifications.RemoveAt(0);

	SpawnNotification(PendingNotification);

	UWorld* World = GetWorld();
	if (!World)
	{
		ProcessNextQueuedNotification();
		return;
	}

	World->GetTimerManager().SetTimer(SpawnQueueTimerHandle, this,
		&ThisClass::ProcessNextQueuedNotification, FMath::Max(SpawnIntervalSeconds, IntroSeconds), false);
}

void USI_ItemAddedNotificationHostWidget::SpawnNotification(const FSI_PendingItemAddedNotification& PendingNotification)
{
	if (!NotificationCanvas) return;
	if (!PendingNotification.WidgetClass) return;

	USI_ItemAddedNotificationWidget* NewWidget =
		CreateWidget<USI_ItemAddedNotificationWidget>(this, PendingNotification.WidgetClass);

	if (!NewWidget) return;

	UCanvasPanelSlot* CanvasSlot = NotificationCanvas->AddChildToCanvas(NewWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(CanvasPosition);
	}

	NewWidget->InitNotification(PendingNotification.NotificationData);

	FSI_ActiveItemAddedNotification ActiveNotification;
	ActiveNotification.Widget = NewWidget;

	ActiveNotifications.Insert(ActiveNotification, 0);

	RefreshStackPositions();

	UWorld* World = GetWorld();
	if (!World) return;

	FTimerDelegate ExitDelegate;
	ExitDelegate.BindUObject(this, &ThisClass::StartExitForWidget, NewWidget);

	World->GetTimerManager().SetTimer(ActiveNotifications[0].ExitTimerHandle, ExitDelegate,
		IntroSeconds + HoldSeconds, false);
}

void USI_ItemAddedNotificationHostWidget::RefreshStackPositions()
{
	int32 StackIndex = 0;

	for (FSI_ActiveItemAddedNotification& ActiveNotification : ActiveNotifications)
	{
		USI_ItemAddedNotificationWidget* Widget = ActiveNotification.Widget;
		if (!Widget) continue;

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			const FVector2D TargetPosition = CanvasPosition + FVector2D(0.f, -StackIndex * StackSpacing);
			CanvasSlot->SetPosition(TargetPosition);
		}

		StackIndex++;
	}
}

void USI_ItemAddedNotificationHostWidget::StartExitForWidget(USI_ItemAddedNotificationWidget* Widget)
{
	if (!Widget) return;

	FSI_ActiveItemAddedNotification* FoundNotification = nullptr;

	for (FSI_ActiveItemAddedNotification& ActiveNotification : ActiveNotifications)
	{
		if (ActiveNotification.Widget == Widget)
		{
			FoundNotification = &ActiveNotification;
			break;
		}
	}

	if (!FoundNotification || FoundNotification->bIsExiting) return;

	FoundNotification->bIsExiting = true;
	Widget->PlayExitAnimation();

	RefreshStackPositions();

	UWorld* World = GetWorld();
	if (!World) return;

	FTimerDelegate RemoveDelegate;
	RemoveDelegate.BindUObject(this, &ThisClass::RemoveNotificationWidget, Widget);

	World->GetTimerManager().SetTimer(FoundNotification->RemoveTimerHandle, RemoveDelegate,
		ExitSeconds, false);
}

void USI_ItemAddedNotificationHostWidget::RemoveNotificationWidget(USI_ItemAddedNotificationWidget* Widget)
{
	if (!Widget) return;

	for (int32 Index = ActiveNotifications.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveNotifications[Index].Widget == Widget)
		{
			ActiveNotifications.RemoveAt(Index);
			break;
		}
	}

	Widget->RemoveFromParent();

	RefreshStackPositions();
}

void USI_ItemAddedNotificationHostWidget::ClearNotificationTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnQueueTimerHandle);

		for (FSI_ActiveItemAddedNotification& ActiveNotification : ActiveNotifications)
		{
			World->GetTimerManager().ClearTimer(ActiveNotification.ExitTimerHandle);
			World->GetTimerManager().ClearTimer(ActiveNotification.RemoveTimerHandle);
		}
	}

	ActiveNotifications.Reset();
	PendingNotifications.Reset();
	bProcessingQueue = false;
}
