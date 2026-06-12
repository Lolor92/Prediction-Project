#include "UI/Popups/SI_SplitStackWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Component/SI_InventoryComponent.h"
#include "Component/SI_InventoryUIManager.h"
#include "Types/Requests/SI_RequestTypes.h"


void USI_SplitStackWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (RemainingQuantitySlider)
	{
		RemainingQuantitySlider->OnValueChanged.RemoveDynamic(this, &ThisClass::HandleRemainingQuantitySliderChanged);
		RemainingQuantitySlider->OnValueChanged.AddDynamic(this, &ThisClass::HandleRemainingQuantitySliderChanged);
	}
	
	if (ConfirmSplitButton)
	{
		ConfirmSplitButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleConfirmSplitClicked);
		ConfirmSplitButton->OnClicked.AddDynamic(this, &ThisClass::HandleConfirmSplitClicked);
	}
}

void USI_SplitStackWidget::InitSplitStackWidget(USI_InventoryUIManager* InInventoryUIManager,
	USI_InventoryComponent* InInventoryComponent, const FGameplayTag& InContainerId, int32 InSlotIndex,
	int32 InCurrentQuantity)
{
	InventoryUIManager = InInventoryUIManager;
	InventoryComponent = InInventoryComponent;
	ContainerId = InContainerId;
	SlotIndex = InSlotIndex;
	CurrentQuantity = InCurrentQuantity;
	
	MaxRemainingQuantity = FMath::Max(1, CurrentQuantity - 1);
	
	if (RemainingQuantitySlider)
	{
		if (CurrentQuantity <= 1)
		{
			RemainingQuantitySlider->SetValue(0.f);
		}
		else
		{
			const int32 DefaultRemainingQuantity = FMath::Clamp(CurrentQuantity / 2, MinRemainingQuantity, MaxRemainingQuantity);
			const float NormalizedValue = MaxRemainingQuantity > MinRemainingQuantity ?
			static_cast<float>(DefaultRemainingQuantity - MinRemainingQuantity) / static_cast<float>(MaxRemainingQuantity - MinRemainingQuantity) : 0.f;
			
			RemainingQuantitySlider->SetValue(NormalizedValue);
		}
	}
	
	RefreshDisplay();
}

void USI_SplitStackWidget::SetPopupScreenPosition(const FVector2D& ScreenPosition)
{
	if (!PopupRoot) return;
	
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), ScreenPosition, PixelPosition, ViewportPosition);
	
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PopupRoot->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D(0, 0));
		CanvasSlot->SetPosition(ViewportPosition);
	}

	CaptureDefaultLayout();
}

void USI_SplitStackWidget::RequestClose()
{
	if (InventoryUIManager)
	{
		InventoryUIManager->CloseSplitStackWidget();
		return;
	}
	
	RemoveFromParent();
}

FReply USI_SplitStackWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton &&
		InMouseEvent.GetEffectingButton() != EKeys::RightMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	if (!PopupRoot)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	const FGeometry& PopupGeometry = PopupRoot->GetCachedGeometry();
	const FVector2D ScreenSpacePosition = InMouseEvent.GetScreenSpacePosition();
	
	const bool bClickedInsidePopup = PopupGeometry.IsUnderLocation(ScreenSpacePosition);
	if (bClickedInsidePopup)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	RequestClose();
	return FReply::Unhandled();
}

UWidget* USI_SplitStackWidget::GetManipulationTarget() const
{
	return DragHandle ? DragHandle.Get() : Cast<UWidget>(PopupRoot);
}

void USI_SplitStackWidget::RefreshDisplay()
{
	const int32 RemainingQuantity = GetRemainingQuantityFromSlider();
	const int32 SplitQuantity = GetSplitQuantity();
	
	if (CurrentQuantityText)
	{
		CurrentQuantityText->SetText(FText::AsNumber(CurrentQuantity));
	}
	
	if (RemainingQuantityText)
	{
		RemainingQuantityText->SetText(FText::AsNumber(RemainingQuantity));
	}
	
	if (SplitQuantityText)
	{
		SplitQuantityText->SetText(FText::AsNumber(SplitQuantity));
	}
}

void USI_SplitStackWidget::HandleRemainingQuantitySliderChanged(float Value)
{
	RefreshDisplay();
}

void USI_SplitStackWidget::HandleConfirmSplitClicked()
{
	if (!InventoryComponent || !ContainerId.IsValid() || SlotIndex < 0 || CurrentQuantity <= 1)
	{
		RequestClose();
		return;
	}
	
	const int32 SplitQuantity = GetSplitQuantity();
	if (SplitQuantity <= 0)
	{
		RequestClose();
		return;
	}
	
	FSI_SplitStackRequest Request;
	Request.ContainerId = ContainerId;
	Request.SlotIndex = SlotIndex;
	Request.SplitQuantity = SplitQuantity;
	
	InventoryComponent->RequestSplitItem(Request);
	RequestClose();
}

int32 USI_SplitStackWidget::GetRemainingQuantityFromSlider() const
{
	if (CurrentQuantity <= 1) return 0;
	
	if (!RemainingQuantitySlider)
	{
		return FMath::Clamp(CurrentQuantity / 2, MinRemainingQuantity, MaxRemainingQuantity);
	}
	
	const float SliderValue = RemainingQuantitySlider->GetValue();
	
	if (MaxRemainingQuantity <= MinRemainingQuantity)
	{
		return MinRemainingQuantity;
	}
	
	const float LerpValue = FMath::Lerp(static_cast<float>(MinRemainingQuantity), static_cast<float>(MaxRemainingQuantity), SliderValue);
	return FMath::Clamp(FMath::RoundToInt(LerpValue), MinRemainingQuantity, MaxRemainingQuantity);
}

int32 USI_SplitStackWidget::GetSplitQuantity() const
{
	const int32 RemainingQuantity = GetRemainingQuantityFromSlider();
	return FMath::Max(0, CurrentQuantity - RemainingQuantity);
}
