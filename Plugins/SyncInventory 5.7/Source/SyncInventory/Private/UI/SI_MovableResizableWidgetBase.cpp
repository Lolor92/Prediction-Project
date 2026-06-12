#include "UI/SI_MovableResizableWidgetBase.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "Input/Reply.h"

void USI_MovableResizableWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	CaptureDefaultLayout();
}

void USI_MovableResizableWidgetBase::StartDragAtScreenPosition(const FVector2D& ScreenPosition)
{
	UCanvasPanelSlot* CanvasSlot = nullptr;
	FVector2D MouseViewport = FVector2D::ZeroVector;
	if (!ResolveCanvasSlot(CanvasSlot) || !ScreenToViewportPosition(ScreenPosition, MouseViewport))
	{
		return;
	}

	DragOffset = MouseViewport - CanvasSlot->GetPosition();
	bIsDragging = true;
	bIsResizing = false;
}

void USI_MovableResizableWidgetBase::StartResizeAtScreenPosition(const FVector2D& ScreenPosition)
{
	UCanvasPanelSlot* CanvasSlot = nullptr;
	FVector2D MouseViewport = FVector2D::ZeroVector;
	if (!ResolveCanvasSlot(CanvasSlot) || !ScreenToViewportPosition(ScreenPosition, MouseViewport))
	{
		return;
	}

	ResizeStartMouseViewport = MouseViewport;
	ResizeStartWindowSize = GetCurrentTargetSize(CanvasSlot);
	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetSize(ResizeStartWindowSize);

	bIsResizing = true;
	bIsDragging = false;
}

void USI_MovableResizableWidgetBase::StopManipulation()
{
	bIsDragging = false;
	bIsResizing = false;
}

bool USI_MovableResizableWidgetBase::GetWindowLayout(FVector2D& OutPosition, FVector2D& OutSize) const
{
	OutPosition = FVector2D::ZeroVector;
	OutSize = FVector2D::ZeroVector;

	UCanvasPanelSlot* CanvasSlot = nullptr;
	if (!ResolveCanvasSlot(CanvasSlot))
	{
		return false;
	}

	OutPosition = CanvasSlot->GetPosition();
	OutSize = GetCurrentTargetSize(CanvasSlot);
	return true;
}

void USI_MovableResizableWidgetBase::SetWindowPosition(const FVector2D& InPosition)
{
	UCanvasPanelSlot* CanvasSlot = nullptr;
	if (!ResolveCanvasSlot(CanvasSlot))
	{
		return;
	}

	CanvasSlot->SetPosition(InPosition);
}

void USI_MovableResizableWidgetBase::CaptureDefaultLayout()
{
	UCanvasPanelSlot* CanvasSlot = nullptr;
	if (!ResolveCanvasSlot(CanvasSlot))
	{
		bHasDefaultLayout = false;
		return;
	}

	DefaultWindowPosition = CanvasSlot->GetPosition();
	DefaultWindowSize = GetCurrentTargetSize(CanvasSlot);
	bDefaultAutoSize = CanvasSlot->GetAutoSize();
	bHasDefaultLayout = true;
}

void USI_MovableResizableWidgetBase::ResetToDefaultLayout()
{
	if (!bHasDefaultLayout)
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot = nullptr;
	if (!ResolveCanvasSlot(CanvasSlot))
	{
		return;
	}

	CanvasSlot->SetAutoSize(bDefaultAutoSize);
	CanvasSlot->SetPosition(DefaultWindowPosition);

	if (!bDefaultAutoSize)
	{
		CanvasSlot->SetSize(DefaultWindowSize);
	}

	StopManipulation();
}

FReply USI_MovableResizableWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == GetDragMouseButton() &&
		ShouldStartDragAtScreenPosition(InMouseEvent.GetScreenSpacePosition()))
	{
		StartDragAtScreenPosition(InMouseEvent.GetScreenSpacePosition());
		if (bIsDragging)
		{
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}

	if (bResizeAnywhereWithRightMouse && InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		UWidget* ManipulationTarget = GetManipulationTarget();
		if (ManipulationTarget && ManipulationTarget->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			StartResizeAtScreenPosition(InMouseEvent.GetScreenSpacePosition());
			if (bIsResizing)
			{
				return FReply::Handled().CaptureMouse(TakeWidget());
			}
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USI_MovableResizableWidgetBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	UCanvasPanelSlot* CanvasSlot = nullptr;
	if (!ResolveCanvasSlot(CanvasSlot))
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	FVector2D MouseViewport = FVector2D::ZeroVector;
	if (!ScreenToViewportPosition(InMouseEvent.GetScreenSpacePosition(), MouseViewport))
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	if (bIsDragging)
	{
		CanvasSlot->SetPosition(MouseViewport - DragOffset);
		return FReply::Handled();
	}

	if (bIsResizing)
	{
		FVector2D NewSize = ResizeStartWindowSize + (MouseViewport - ResizeStartMouseViewport);
		NewSize.X = FMath::Max(NewSize.X, MinWindowSize.X);
		NewSize.Y = FMath::Max(NewSize.Y, MinWindowSize.Y);

		if (MaxWindowSize.X > 0.f)
		{
			NewSize.X = FMath::Min(NewSize.X, MaxWindowSize.X);
		}

		if (MaxWindowSize.Y > 0.f)
		{
			NewSize.Y = FMath::Min(NewSize.Y, MaxWindowSize.Y);
		}

		CanvasSlot->SetSize(NewSize);
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply USI_MovableResizableWidgetBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging || bIsResizing)
	{
		StopManipulation();
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FKey USI_MovableResizableWidgetBase::GetDragMouseButton() const
{
	return EKeys::LeftMouseButton;
}

bool USI_MovableResizableWidgetBase::ShouldStartDragAtScreenPosition(const FVector2D& ScreenPosition) const
{
	UWidget* ManipulationTarget = GetManipulationTarget();
	if (!ManipulationTarget)
	{
		return false;
	}

	const FGeometry& TargetGeometry = ManipulationTarget->GetCachedGeometry();
	if (!TargetGeometry.IsUnderLocation(ScreenPosition))
	{
		return false;
	}

	switch (DragMode)
	{
	case ESI_WidgetDragMode::Anywhere:
		return true;

	case ESI_WidgetDragMode::TopBarOnly:
		{
			const FVector2D LocalMousePosition = TargetGeometry.AbsoluteToLocal(ScreenPosition);
			return LocalMousePosition.Y >= 0.f && LocalMousePosition.Y <= TitleBarDragHeight;
		}

	case ESI_WidgetDragMode::Disabled:
	default:
		return false;
	}
}

bool USI_MovableResizableWidgetBase::ResolveCanvasSlot(UCanvasPanelSlot*& OutCanvasSlot) const
{
	UWidget* ManipulationTarget = GetManipulationTarget();
	OutCanvasSlot = ManipulationTarget ? Cast<UCanvasPanelSlot>(ManipulationTarget->Slot) : nullptr;
	return OutCanvasSlot != nullptr;
}

bool USI_MovableResizableWidgetBase::ScreenToViewportPosition(const FVector2D& ScreenPosition, FVector2D& OutViewportPosition) const
{
	FVector2D PixelPosition = FVector2D::ZeroVector;
	OutViewportPosition = FVector2D::ZeroVector;

	if (!GetWorld())
	{
		return false;
	}

	USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), ScreenPosition, PixelPosition, OutViewportPosition);
	return true;
}

FVector2D USI_MovableResizableWidgetBase::GetCurrentTargetSize(const UCanvasPanelSlot* CanvasSlot) const
{
	if (!CanvasSlot)
	{
		return FVector2D::ZeroVector;
	}

	if (UWidget* ManipulationTarget = GetManipulationTarget())
	{
		const FVector2D GeometrySize = ManipulationTarget->GetCachedGeometry().GetLocalSize();
		if (!GeometrySize.IsNearlyZero())
		{
			return GeometrySize;
		}

		const FVector2D DesiredSize = ManipulationTarget->GetDesiredSize();
		if (!DesiredSize.IsNearlyZero())
		{
			return DesiredSize;
		}
	}

	return CanvasSlot->GetSize();
}
